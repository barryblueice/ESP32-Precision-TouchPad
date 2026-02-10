#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "i2c/goodix/goodix_i2c.h"
#include "i2c/I2C_HID_Report.h"

#include "usb/usbhid.h"

#include <math.h>

static const char *TAG = "GOODIX_PTP";

QueueHandle_t tp_queue = NULL;
QueueHandle_t mouse_queue = NULL;
QueueSetHandle_t main_queue_set = NULL;
volatile uint8_t current_mode = MOUSE_MODE;

#define I2C_ADDR 0x2c
#define RST_IO   3
#define INT_IO   4

i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;

TaskHandle_t tp_read_task_handle = NULL;

#define TAP_MOVE_THRESHOLD 10
#define TAP_TIME_THRESHOLD 150
#define DOUBLE_TAP_WINDOW  50
#define MULTI_TAP_JOIN_MS 30

esp_err_t goodix_activate_ptp() {
    uint8_t payload[] = {
        0x05, 0x00,             // Command Register
        0x33, 0x03,             // SET_REPORT Feature ID 03
        0x06, 0x00,
        0x05, 0x00,             // Usage Page: Digitizer
        0x03, 0x03, 0x00        // Value: Enable PTP Mode
    };
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

esp_err_t goodix_activate_mouse() {
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

void goodix_i2c_init(void) {
    gpio_set_direction(RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 6,
        .sda_io_num = 7,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 400000,
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
#define SETTLING_MS 50
#define FILTER_ALPHA 0.5f

typedef enum {
    TOUCH_IDLE,
    TOUCH_TAP_CANDIDATE,
    TOUCH_DRAG
} touch_state_t;

static uint16_t get_median(uint16_t n1, uint16_t n2, uint16_t n3) {
    if ((n1 > n2) ^ (n1 > n3)) return n1;
    else if ((n2 > n1) ^ (n2 > n3)) return n2;
    else return n3;
}

void goodix_i2c_task(void *arg) {
    static uint16_t last_raw_x[5] = {0};
    static uint16_t last_raw_y[5] = {0};
    static uint16_t origin_x[5] = {0};
    static uint16_t origin_y[5] = {0};
    static int64_t last_report_time = 0;
    static bool tap_frozen[5] = {false};
    static touch_state_t touch_state[5] = {0};
    static uint32_t filtered_x[5] = {0};
    static uint32_t filtered_y[5] = {0};
    
    #define HISTORY_LEN 3
    static uint16_t raw_x_history[5][HISTORY_LEN] = {0};
    static uint16_t raw_y_history[5][HISTORY_LEN] = {0};
    static int consecutive_errors[5] = {0}; 

    uint8_t data[64];
    const uint16_t JUMP_THRESHOLD = 800;

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1));

        tp_multi_msg_t tp_current_state = {0};
        mouse_msg_t mouse_current_state = {0};
        bool has_data = false;
        int64_t now = esp_timer_get_time() / 1000;

        if (now - last_report_time > SETTLING_MS) {
            for (int i = 0; i < 5; i++) {
                last_raw_x[i] = 0; last_raw_y[i] = 0;
                origin_x[i] = 0; origin_y[i] = 0;
                consecutive_errors[i] = 0;
                memset(raw_x_history[i], 0, sizeof(raw_x_history[i]));
            }
        }

        int safety = 10;
        while (gpio_get_level(INT_IO) == 0 && safety-- > 0) {
            if (i2c_master_receive(dev_handle, data, sizeof(data), pdMS_TO_TICKS(5)) == ESP_OK) {
                if (data[2] == 0x04) {
                    current_mode = PTP_MODE;
                    tp_current_state.scan_time = data[28] | (data[29] << 8);
                    tp_current_state.button_mask = (data[31] & 0x01) ? 0x01 : 0x00;
                    tp_current_state.actual_count = data[30];

                    for (int id = 0; id < 5; id++) {
                        uint8_t *f_ptr = &data[3 + (id * 5)];
                        bool is_valid_touch = (f_ptr[0] & 0x01);
                        uint16_t rx = f_ptr[1] | (f_ptr[2] << 8);
                        uint16_t ry = f_ptr[3] | (f_ptr[4] << 8);

                        if (!is_valid_touch || rx == 0 || ry == 0) {
                            last_raw_x[id] = 0; last_raw_y[id] = 0;
                            consecutive_errors[id] = 0;
                            memset(raw_x_history[id], 0, sizeof(raw_x_history[id]));
                            tp_current_state.fingers[id].tip_switch = 0;
                            continue;
                        }

                        for (int h = 0; h < HISTORY_LEN - 1; h++) {
                            raw_x_history[id][h] = raw_x_history[id][h+1];
                            raw_y_history[id][h] = raw_y_history[id][h+1];
                        }
                        raw_x_history[id][HISTORY_LEN-1] = rx;
                        raw_y_history[id][HISTORY_LEN-1] = ry;

                        if (raw_x_history[id][0] == 0) continue;

                        uint16_t mx = get_median(raw_x_history[id][0], raw_x_history[id][1], raw_x_history[id][2]);
                        uint16_t my = get_median(raw_y_history[id][0], raw_y_history[id][1], raw_y_history[id][2]);

                        int16_t predict_x = raw_x_history[id][HISTORY_LEN-1];
                        int16_t predict_y = raw_y_history[id][HISTORY_LEN-1];
                        int dx_sum = 0, dy_sum = 0, count_valid = 0;
                        for (int h = 1; h < HISTORY_LEN; h++) {
                            if (raw_x_history[id][h-1] && raw_x_history[id][h]) {
                                dx_sum += raw_x_history[id][h] - raw_x_history[id][h-1];
                                dy_sum += raw_y_history[id][h] - raw_y_history[id][h-1];
                                count_valid++;
                            }
                        }

                        if (count_valid > 0) {
                            predict_x += dx_sum / count_valid;
                            predict_y += dy_sum / count_valid;

                            const int MAX_JUMP2_STRICT = 300 * 300;
                            int dx_jump = mx - predict_x;
                            int dy_jump = my - predict_y;

                            if (dx_jump*dx_jump + dy_jump*dy_jump > MAX_JUMP2_STRICT) {
                                if (consecutive_errors[id] < 2) { 
                                    mx = last_raw_x[id] ? last_raw_x[id] : mx;
                                    my = last_raw_y[id] ? last_raw_y[id] : my;
                                    consecutive_errors[id]++;
                                } else {
                                    consecutive_errors[id] = 0;
                                }
                            } else {
                                consecutive_errors[id] = 0;
                            }
                        }

                        if (last_raw_x[id] == 0) {
                            filtered_x[id] = mx << 8; filtered_y[id] = my << 8;
                            origin_x[id] = mx; origin_y[id] = my;
                        } else {
                            int vx = mx - last_raw_x[id];
                            int vy = my - last_raw_y[id];
                            int speed = abs(vx) + abs(vy);
                            uint32_t alpha = (speed < 3) ? 64 : (speed < 12 ? 115 : 218);
                            filtered_x[id] = (alpha * (mx << 8) + (256 - alpha) * filtered_x[id]) >> 8;
                            filtered_y[id] = (alpha * (my << 8) + (256 - alpha) * filtered_y[id]) >> 8;
                        }

                        uint16_t fx = (uint16_t)(filtered_x[id] >> 8);
                        uint16_t fy = (uint16_t)(filtered_y[id] >> 8);

                        int dx_org = abs((int)mx - (int)origin_x[id]);
                        int dy_org = abs((int)my - (int)origin_y[id]);

                        if (touch_state[id] == TOUCH_TAP_CANDIDATE && (dx_org > TAP_DEADZONE || dy_org > TAP_DEADZONE)) {
                            touch_state[id] = TOUCH_DRAG;
                            tap_frozen[id] = false;
                        }

                        if (touch_state[id] == TOUCH_TAP_CANDIDATE) {
                            if (!tap_frozen[id]) {
                                tap_frozen[id] = true;
                                filtered_x[id] = origin_x[id] << 8;
                                filtered_y[id] = origin_y[id] << 8;
                            }
                            tp_current_state.fingers[id].x = origin_x[id];
                            tp_current_state.fingers[id].y = origin_y[id];
                        } else {
                            tap_frozen[id] = false;
                            tp_current_state.fingers[id].x = fx;
                            tp_current_state.fingers[id].y = fy;
                        }

                        if (!tap_frozen[id] && last_raw_x[id] != 0 && abs((int)mx - (int)last_raw_x[id]) > JUMP_THRESHOLD) {
                            tp_current_state.fingers[id].x = last_raw_x[id];
                            tp_current_state.fingers[id].y = last_raw_y[id];
                        } else {
                            last_raw_x[id] = mx;
                            last_raw_y[id] = my;
                        }

                        tp_current_state.fingers[id].tip_switch = 1;
                        tp_current_state.fingers[id].contact_id = id;
                        has_data = true;
                    }
                } else if (data[2] == 0x01) {
                    current_mode = MOUSE_MODE;
                    mouse_current_state.x = (int8_t)data[4];
                    mouse_current_state.y = (int8_t)data[5];
                    mouse_current_state.buttons = data[3];
                    has_data = true;
                }
            }
        }

        if (has_data) {
            last_report_time = now;
            if (current_mode == PTP_MODE) {
                if (data[3] == 0x01) tp_current_state.actual_count = 0;
                xQueueOverwrite(tp_queue, &tp_current_state);
            } else if (current_mode == MOUSE_MODE) {
                xQueueOverwrite(mouse_queue, &mouse_current_state);
            }
        }
    }
}