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

static const char *TAG = "ELAN_PTP";

QueueHandle_t tp_queue = NULL;

#define I2C_ADDR 0x15
#define RST_IO   6
#define INT_IO   7

i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;

TaskHandle_t tp_read_task_handle = NULL;

static esp_err_t elan_activate_ptp() {
    uint8_t payload[] = {
        0x05, 0x00,             // Command Register
        0x33, 0x03,             // SET_REPORT Feature ID 03
        0x06, 0x00,
        0x05, 0x00,             // Usage Page: Digitizer
        0x03, 0x03, 0x00        // Value: Enable PTP Mode
    };
    ESP_LOGI(TAG, "Activating PTP Mode...");
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

static void elan_i2c_init(void) {
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

    elan_activate_ptp();
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_direction(INT_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INT_IO, GPIO_PULLUP_ONLY);
}

// PTP 数据解析 (Tip/ID/XY)
static void parse_ptp_report(uint8_t *data, int len, finger_layout_t layout, tp_multi_msg_t *msg_out)  {
    uint8_t report_id = data[2];
    if (report_id != 0x04) return;

    uint8_t status = data[3];
    uint8_t contact_id = (status & 0xF0) >> 4;
    bool tip = (status & 0x01);
    uint16_t raw_x = data[4] | (data[5] << 8);
    uint16_t raw_y = data[6] | (data[7] << 8);
    
    bool physical_pressed = (data[11] == 0x81);
    static uint8_t locked_button = 0;
    static bool is_detecting_click = false;

    if (physical_pressed) {
        if (!is_detecting_click) {
            is_detecting_click = true;
            locked_button = (raw_x > 1700) ? 0x02 : 0x01;
        }
        msg_out->button_mask = locked_button;
    } else {
        is_detecting_click = false;
        locked_button = 0;
        msg_out->button_mask = 0;
    }

    if (contact_id < 5) {
        msg_out->fingers[contact_id].x = raw_x;
        msg_out->fingers[contact_id].y = raw_y;
        msg_out->fingers[contact_id].tip_switch = tip ? 1 : 0;
        msg_out->fingers[contact_id].contact_id = contact_id;
    }

    uint8_t count = 0;
    for (int i = 0; i < 5; i++) {
        if (msg_out->fingers[i].tip_switch) {
            count++;
        }
    }
    msg_out->actual_count = count;
}

void elan_i2c_task(void *arg) {
    elan_i2c_init();
    tp_multi_msg_t msg_result;
    uint8_t locked_button = 0;
    memset(&msg_result, 0, sizeof(tp_multi_msg_t));

    // 记录每个 ID 最后一次更新的时间（用于超时强制释放）
    uint32_t last_update_ms[5] = {0};

    while (1) {
        bool has_new_packet = false;

        // 1. 读取当前中断周期内的所有包
        while (gpio_get_level(INT_IO) == 0) {
            uint8_t data[64];
            if (i2c_master_receive(dev_handle, data, sizeof(data), pdMS_TO_TICKS(5)) == ESP_OK) {
                if (data[2] == 0x04) { 
                    uint8_t status = data[3];
                    uint8_t contact_id = (status & 0xF0) >> 4;
                    uint16_t raw_x = data[4] | (data[5] << 8);
                    uint16_t raw_y = data[6] | (data[7] << 8);
                    bool tip = (status & 0x01);

                    if (contact_id < 5) {
                        msg_result.fingers[contact_id].tip_switch = tip ? 1 : 0;
                        msg_result.fingers[contact_id].x = raw_x;
                        msg_result.fingers[contact_id].y = raw_y;
                        msg_result.fingers[contact_id].contact_id = contact_id;
                        
                        last_update_ms[contact_id] = xTaskGetTickCount() * portTICK_PERIOD_MS;
                        has_new_packet = true;
                    }
                    
                    if (data[11] == 0x81) {
                        locked_button = (raw_x > 1800) ? 0x02 : 0x01;
                        msg_result.button_mask = 0x01;
                    } else {
                        msg_result.button_mask = 0;
                    }
                }
            }
        }

        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        for (int i = 0; i < 5; i++) {
            if (msg_result.fingers[i].tip_switch && (now - last_update_ms[i] > 100)) {
                msg_result.fingers[i].tip_switch = 0;
                has_new_packet = true;
            }
        }

        if (has_new_packet) {
            uint8_t count = 0;
            for (int i = 0; i < 5; i++) {
                if (msg_result.fingers[i].tip_switch) count++;
            }
            msg_result.actual_count = count;
            xQueueSend(tp_queue, &msg_result, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}

