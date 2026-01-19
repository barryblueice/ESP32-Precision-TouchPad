#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "i2c/elan_i2c.h"

#include "usb/usbhid.h"

#include <math.h>

static const char *TAG = "ELAN_PTP";

QueueHandle_t tp_queue = NULL;
QueueHandle_t mouse_queue = NULL;
QueueSetHandle_t main_queue_set = NULL;
volatile uint8_t current_mode = MOUSE_MODE;

#define I2C_ADDR 0x15
#define RST_IO   6
#define INT_IO   7

i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;

TaskHandle_t tp_read_task_handle = NULL;

#define TAP_MOVE_THRESHOLD 10
#define TAP_TIME_THRESHOLD 150
#define DOUBLE_TAP_WINDOW  50
#define MULTI_TAP_JOIN_MS 30

esp_err_t elan_activate_ptp() {
    uint8_t payload[] = {
        0x05, 0x00,             // Command Register
        0x33, 0x03,             // SET_REPORT Feature ID 03
        0x06, 0x00,
        0x05, 0x00,             // Usage Page: Digitizer
        0x03, 0x03, 0x00        // Value: Enable PTP Mode
    };
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

esp_err_t elan_activate_mouse() {
    uint8_t payload[] = {
        0x05, 0x00,
        0x33, 0x03,
        0x06, 0x00,
        0x05, 0x00,
        0x01, 0x00, 0x00        // Value: Enable Mouse Mode
    };
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

typedef struct {
    uint8_t tip_offset;
    uint8_t id_offset;
    uint8_t x_offset;
    uint8_t y_offset;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t block_size;
    bool nibble_packed;
} finger_layout_t;

void elan_i2c_init(void) {
    gpio_set_direction(RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 9,
        .sda_io_num = 8,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 800000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    uint8_t pwr_on[] = {0x05, 0x00, 0x08, 0x00};
    i2c_master_transmit(dev_handle, pwr_on, 4, 100);
    vTaskDelay(pdMS_TO_TICKS(20));
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_direction(INT_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INT_IO, GPIO_PULLUP_ONLY);
}

#define TAP_DEADZONE 30
#define SETTLING_MS 15
#define FILTER_ALPHA 0.5f

static float filtered_x[5] = {0};
static float filtered_y[5] = {0};

typedef enum {
    TOUCH_IDLE,
    TOUCH_TAP_CANDIDATE,
    TOUCH_DRAG
} touch_state_t;


void elan_i2c_task(void *arg) {

    static uint16_t last_raw_x[5] = {0};
    static uint16_t last_raw_y[5] = {0};
    static uint16_t origin_x[5] = {0};
    static uint16_t origin_y[5] = {0};
    static int64_t last_report_time = 0;
    static int64_t first_touch_time = 0;
    static bool tap_frozen[5] = {false};
    static touch_state_t touch_state[5] = {0};
    
    uint8_t data[64];
    const uint16_t JUMP_THRESHOLD = 800;
    const int64_t STALE_MS = 50;

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)); 

        tp_multi_msg_t tp_current_state = {0}; 
        mouse_msg_t mouse_current_state = {0};
        bool has_data = false;
        int64_t now = esp_timer_get_time() / 1000;

        if (now - last_report_time > STALE_MS) {
            for (int i = 0; i < 5; i++) { 
                last_raw_x[i] = 0; last_raw_y[i] = 0; 
                origin_x[i] = 0; origin_y[i] = 0;
            }
        }

        int safety = 10;
        while (gpio_get_level(INT_IO) == 0 && safety-- > 0) {
            if (i2c_master_receive(dev_handle, data, sizeof(data), pdMS_TO_TICKS(5)) == ESP_OK) {
                tp_current_state.button_mask = (data[11] & 0x01) ? 0x01 : 0x00;

                // ESP_LOG_BUFFER_HEX(TAG, data, 12);

                if (data[2] == 0x04) {

                    current_mode = PTP_MODE;

                    uint8_t status = data[3];
                    uint8_t id = (status & 0xF0) >> 4;

                    if (id < 5) {

                        uint8_t status = data[3];
                        uint8_t id = (status & 0xF0) >> 4;
                        bool tip = (status & 0x01);
                        bool confidence = (status & 0x02);
                        
                        bool is_valid_touch = tip && confidence;

                        uint16_t rx = data[4] | (data[5] << 8);
                        uint16_t ry = data[6] | (data[7] << 8);

                        if (is_valid_touch && rx > 0 && ry > 0) {

                            if (last_raw_x[id] == 0) {
                                touch_state[id] = TOUCH_TAP_CANDIDATE;
                                origin_x[id] = rx;
                                origin_y[id] = ry;
                                filtered_x[id] = (float)rx;
                                filtered_y[id] = (float)ry;

                                if (first_touch_time == 0)
                                    first_touch_time = now;
                            } else {
                                int vx = rx - last_raw_x[id];
                                int vy = ry - last_raw_y[id];
                                int alpha_speed = abs(vx) + abs(vy);

                                float dynamic_alpha;
                                if (alpha_speed < 3)
                                    dynamic_alpha = 0.25f;
                                else if (alpha_speed < 12)
                                    dynamic_alpha = 0.45f;
                                else
                                    dynamic_alpha = 0.85f;

                                filtered_x[id] = dynamic_alpha * rx + (1.0f - dynamic_alpha) * filtered_x[id];
                                filtered_y[id] = dynamic_alpha * ry + (1.0f - dynamic_alpha) * filtered_y[id];
                            }

                            uint16_t fx = (uint16_t)filtered_x[id];
                            uint16_t fy = (uint16_t)filtered_y[id];

                            int dx_raw = rx - last_raw_x[id];
                            int dy_raw = ry - last_raw_y[id];

                            if (abs(dx_raw) > abs(dy_raw) * 2 && abs(dy_raw) < 6) {
                                ry = last_raw_y[id];
                            } else if (abs(dy_raw) > abs(dx_raw) * 2 && abs(dx_raw) < 6) {
                                rx = last_raw_x[id];
                            }

                            int dx = abs((int)rx - (int)origin_x[id]);
                            int dy = abs((int)ry - (int)origin_y[id]);

                            if (touch_state[id] == TOUCH_TAP_CANDIDATE &&
                                (dx > TAP_DEADZONE || dy > TAP_DEADZONE)) {
                                touch_state[id] = TOUCH_DRAG;
                                tap_frozen[id] = false;
                            }

                            if (touch_state[id] == TOUCH_TAP_CANDIDATE) {
                                if (!tap_frozen[id]) {
                                    tap_frozen[id] = true;
                                    filtered_x[id] = origin_x[id];
                                    filtered_y[id] = origin_y[id];
                                }
                                tp_current_state.fingers[id].x = origin_x[id];
                                tp_current_state.fingers[id].y = origin_y[id];
                            } else {
                                tap_frozen[id] = false;
                                tp_current_state.fingers[id].x = fx;
                                tp_current_state.fingers[id].y = fy;
                            }

                            int sum_x = 0, sum_y = 0, count = 0;
                            for (int i = 0; i < 5; i++) {
                                if (tap_frozen[i]) {
                                    sum_x += origin_x[i];
                                    sum_y += origin_y[i];
                                    count++;
                                }
                            }
                            if (count > 1) {
                                int avg_x = sum_x / count;
                                int avg_y = sum_y / count;
                                for (int i = 0; i < 5; i++) {
                                    if (tap_frozen[i]) {
                                        origin_x[i] = avg_x;
                                        origin_y[i] = avg_y;
                                    }
                                }
                            }

                            if (!tap_frozen[id] &&
                                last_raw_x[id] != 0 &&
                                abs((int)rx - (int)last_raw_x[id]) > JUMP_THRESHOLD) {
                                tp_current_state.fingers[id].x = last_raw_x[id];
                                tp_current_state.fingers[id].y = last_raw_y[id];
                            }

                            last_raw_x[id] = rx;
                            last_raw_y[id] = ry;

                            tp_current_state.fingers[id].tip_switch = 1;
                            tp_current_state.fingers[id].contact_id = id;
                            has_data = true;
                        } else {
                            tp_current_state.fingers[id].tip_switch = 0;
                            tp_current_state.fingers[id].x = last_raw_x[id];
                            tp_current_state.fingers[id].y = last_raw_y[id];

                            last_raw_x[id] = 0;
                            last_raw_y[id] = 0;
                            origin_x[id] = 0;
                            origin_y[id] = 0;
                            filtered_x[id] = 0;
                            filtered_y[id] = 0;
                            tap_frozen[id] = false;
                            touch_state[id] = TOUCH_IDLE;

                            has_data = true;
                        }
                    }
                    
                } else if (data[2] == 0x01) {

                        current_mode = MOUSE_MODE;

                        // ESP_LOG_BUFFER_HEX(TAG, data, sizeof(data));

                        // ESP_LOGI(TAG, "X: %d, y:%d",(int8_t)data[4], (int8_t)data[5]);

                        mouse_current_state.x = (int8_t)data[4];
                        mouse_current_state.y = (int8_t)data[5];

                        mouse_current_state.buttons = data[3];
                } else {
                    break;
                }
            }
        }

        if (current_mode == PTP_MODE) {
            if (has_data || tp_current_state.button_mask) {
                uint8_t active_count = 0;
                for (int i = 0; i < 5; i++) {
                    if (tp_current_state.fingers[i].tip_switch) active_count++;
                }
                tp_current_state.actual_count = active_count;

                if (active_count == 0) first_touch_time = 0;

                last_report_time = now;
                xQueueOverwrite(tp_queue, &tp_current_state);
            }
        } else if (current_mode == MOUSE_MODE) {
            xQueueOverwrite(mouse_queue, &mouse_current_state);
        }
    }
}
