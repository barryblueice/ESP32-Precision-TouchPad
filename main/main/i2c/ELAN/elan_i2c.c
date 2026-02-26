#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "i2c/ELAN/elan_i2c.h"
#include "i2c/I2C_HID_Report.h"

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
#define STALE_MS     50

typedef enum {
    TOUCH_IDLE,
    TOUCH_TAP_CANDIDATE,
    TOUCH_DRAG
} touch_state_t;

void elan_capture_task(void *arg) {
    tp_raw_packet_t packet;
    while (1) {
        // 优先检查硬件中断引脚
        if (gpio_get_level(INT_IO) == 0) {
            if (i2c_master_receive(dev_handle, packet.raw_data, 64, pdMS_TO_TICKS(5)) == ESP_OK) {
                packet.timestamp = esp_timer_get_time() / 1000;
                xQueueSend(tp_raw_pool, &packet, 0);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

typedef enum { 
    GATE_IDLE = 0, 
    GATE_WAIT_SECOND, 
    GATE_DUAL_ACTIVE } 
    id0_gate_state_t;

void finalize_and_send_frame(tp_multi_msg_t *frame, id0_gate_state_t *state, int64_t *start_time, bool *has_id0, tp_finger_t *cache0, int64_t now) {
    uint8_t active_count = 0;
    for (int i = 0; i < 5; i++) {
        if (frame->fingers[i].tip_switch) active_count++;
    }

    static uint8_t last_frame_active_count = 0;

    if (active_count == 0 && last_frame_active_count == 0 && frame->button_mask == 0) {
        return;
    }

    switch (*state) {
        case GATE_IDLE:
            if (active_count == 1 && frame->fingers[0].tip_switch && *has_id0) {
                *state = GATE_WAIT_SECOND;
                *start_time = now;
                frame->fingers[0].tip_switch = 0;
            } else if (*has_id0) {
                frame->fingers[0] = *cache0;
            }
            break;

        case GATE_WAIT_SECOND:
            if (active_count >= 2) {
                frame->fingers[0] = *cache0;
                *state = GATE_DUAL_ACTIVE;
            } else if ((now - *start_time) > 8) {
                frame->fingers[0] = *cache0;
                *state = GATE_IDLE;
            } else {
                frame->fingers[0].tip_switch = 0;
            }
            break;

        case GATE_DUAL_ACTIVE:
            if (active_count <= 1) {
                *state = GATE_IDLE;
                *has_id0 = false;
            } else {
                frame->fingers[0] = *cache0;
            }
            break;
    }

    uint8_t final_count = 0;
    for (int i = 0; i < 5; i++) {
        if (frame->fingers[i].tip_switch) final_count++;
    }
    frame->actual_count = final_count;

    xQueueOverwrite(tp_queue, frame);

    last_frame_active_count = active_count;
}

void elan_processing_task(void *arg) {
    static uint16_t last_raw_x[5] = {0}, last_raw_y[5] = {0};
    static uint32_t filtered_x[5] = {0}, filtered_y[5] = {0};
    
    static tp_multi_msg_t shadow_frame = {0};
    static uint8_t received_mask = 0;

    static tp_finger_t cached_id0 = {0};
    static bool has_cached_id0 = false;
    static id0_gate_state_t gate_state = GATE_IDLE;
    static int64_t gate_start_time = 0;

    tp_raw_packet_t packet;

    while (1) {
        if (xQueueReceive(tp_raw_pool, &packet, portMAX_DELAY)) {
            uint8_t *data = packet.raw_data;
            int64_t now = packet.timestamp;

            if (data[2] == 0x04) {
                uint8_t status = data[3];
                uint8_t id = (status & 0xF0) >> 4;
                if (id >= 5) continue;

                if (received_mask & (1 << id)) {
                    finalize_and_send_frame(&shadow_frame, &gate_state, &gate_start_time, &has_cached_id0, &cached_id0, now);
                    memset(&shadow_frame, 0, sizeof(tp_multi_msg_t));
                    received_mask = 0;
                }

                bool tip = (status & 0x01);
                bool confidence = (status & 0x02);
                uint16_t rx = data[4] | (data[5] << 8);
                uint16_t ry = data[6] | (data[7] << 8);

                if (tip && confidence && rx > 0 && ry > 0) {
                    if (last_raw_x[id] == 0) {
                        filtered_x[id] = rx << 8; filtered_y[id] = ry << 8;
                    } else {
                        int vx = rx - last_raw_x[id], vy = ry - last_raw_y[id];
                        uint32_t alpha = (abs(vx) + abs(vy) < 3) ? 64 : (abs(vx) + abs(vy) < 12 ? 115 : 218);
                        filtered_x[id] = (alpha * (rx << 8) + (256 - alpha) * filtered_x[id]) >> 8;
                        filtered_y[id] = (alpha * (ry << 8) + (256 - alpha) * filtered_y[id]) >> 8;
                    }

                    shadow_frame.fingers[id].x = (uint16_t)(filtered_x[id] >> 8);
                    shadow_frame.fingers[id].y = (uint16_t)(filtered_y[id] >> 8);
                    shadow_frame.fingers[id].tip_switch = 1;
                    shadow_frame.fingers[id].contact_id = id;

                    if (id == 0) {
                        cached_id0 = shadow_frame.fingers[0];
                        has_cached_id0 = true;
                    }
                    last_raw_x[id] = rx; last_raw_y[id] = ry;
                } else {
                    shadow_frame.fingers[id].tip_switch = 0;
                    last_raw_x[id] = 0;
                    if (id == 0) has_cached_id0 = false;
                }

                shadow_frame.button_mask = (data[11] & 0x01);
                received_mask |= (1 << id);
            }
        }
    }
}