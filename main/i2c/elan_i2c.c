#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

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

static finger_layout_t parse_finger_layout(const uint8_t *desc, int len)
{
    finger_layout_t layout = {0};

    for (int i = 0; i < len-1; i++) {
        if (desc[i] == 0x09 && desc[i+1] == 0x30) {
            layout.nibble_packed = true;
            layout.tip_offset = 0;
            layout.id_offset = 1;
            layout.x_offset = 2;
            layout.y_offset = 4;
            layout.block_size = 5;
            layout.x_size = 12;
            layout.y_size = 12;
            return layout;
        }
    }

    layout.nibble_packed = false;
    layout.tip_offset = 0;
    layout.id_offset = 1;
    layout.x_offset = 2;
    layout.y_offset = 4;
    layout.block_size = 6;
    layout.x_size = 16;
    layout.y_size = 16;
    return layout;
}

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
        .scl_speed_hz = 1000000,
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
static void parse_ptp_data(uint8_t *buf, tp_multi_msg_t *msg_out) {
    if (buf[2] != 0x04) return;

    uint8_t status = buf[3];
    bool confidence = (status >> 0) & 0x01;
    bool tip = (status >> 1) & 0x01;
    uint8_t contact_id = (status >> 4) & 0x0F;

    if (confidence) {
        msg_out->actual_count = tip ? 1 : 0;
        msg_out->fingers[0].tip_switch = tip ? 1 : 0;
        msg_out->fingers[0].contact_id = contact_id;

        if (tip) {
            msg_out->fingers[0].x = buf[4] | (buf[5] << 8);
            msg_out->fingers[0].y = buf[6] | (buf[7] << 8);
        }
    }
}

void elan_i2c_task(void *arg) {
    
    elan_i2c_init();
    tp_interrupt_init();

    finger_layout_t layout = parse_finger_layout(hid_report_descriptor, 150);

    ESP_LOGI("DEBUG", "--- Finger Layout Derived from HID Desc ---");
    ESP_LOGI("DEBUG", "Tip Offset: %d", layout.tip_offset);
    ESP_LOGI("DEBUG", "ID Offset:  %d", layout.id_offset);
    ESP_LOGI("DEBUG", "X Offset:   %d, Size: %d", layout.x_offset, layout.x_size);
    ESP_LOGI("DEBUG", "Y Offset:   %d, Size: %d", layout.y_offset, layout.y_size);
    ESP_LOGI("DEBUG", "Block Size: %d", layout.block_size);
    ESP_LOGI("DEBUG", "Nibble Packed: %s", layout.nibble_packed ? "YES" : "NO");

    if (tp_read_task_handle == NULL) {
        tp_read_task_handle = xTaskGetCurrentTaskHandle();
    }

    uint8_t rx_buf[64];
    tp_multi_msg_t msg;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (gpio_get_level(INT_IO) == 0) {
            if (i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(5)) == ESP_OK) {
                
                parse_ptp_data(rx_buf, &msg);
                xQueueSend(tp_queue, &msg, 0);
            } else {
                break;
            }
        }
    }
}