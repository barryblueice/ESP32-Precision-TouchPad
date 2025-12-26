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
static void parse_ptp_data(uint8_t *data, int len, finger_layout_t layout, tp_multi_msg_t *msg_out) 
{
    uint8_t report_id = data[2];
    if (report_id != 0x04) return;

    static bool is_detecting_click = false; 
    static uint8_t locked_button = 0;

    memset(msg_out, 0, sizeof(tp_multi_msg_t));

    uint16_t raw_x = data[4] | (data[5] << 8);
    uint16_t raw_y = data[6] | (data[7] << 8);
    
    bool physical_pressed = (data[11] == 0x81);
    uint8_t status = data[3];
    bool tip = (status & 0x01);

    if (physical_pressed) {
        if (!is_detecting_click) {
            is_detecting_click = true;
            if (raw_x > 1700) {
                locked_button = 0x02;
            } else {
                locked_button = 0x01;
            }
        }
        msg_out->button_mask = locked_button;
    } else {
        is_detecting_click = false;
        locked_button = 0;
        msg_out->button_mask = 0;
    }

    if (tip) {
        msg_out->actual_count = 1;
        msg_out->fingers[0].tip_switch = 1;
        msg_out->fingers[0].x = raw_x;
        msg_out->fingers[0].y = raw_y;
    }

    if (physical_pressed) {
        ESP_LOGD(TAG, "[CLICK] BTN:%d | X:%d | Raw11:%02X", msg_out->button_mask, raw_x, data[11]);
    }
}

void elan_i2c_task(void *arg) {
    elan_i2c_init();
    
    tp_interrupt_init(); 

    finger_layout_t layout = parse_finger_layout(hid_report_descriptor, 150);
    ESP_LOGI(TAG, "Layout Init - X Size: %d, Block: %d", layout.x_size, layout.block_size);

    if (tp_read_task_handle == NULL) {
        tp_read_task_handle = xTaskGetCurrentTaskHandle();
    }

    uint8_t rx_buf[64];
    tp_multi_msg_t msg;

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        int retry_count = 0;
        while (gpio_get_level(INT_IO) == 0 && retry_count < 10) {
            esp_err_t err = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(10));
            
            if (err == ESP_OK) {
                parse_ptp_data(rx_buf, sizeof(rx_buf), layout, &msg);
                
                if (tp_queue != NULL) {
                    xQueueSend(tp_queue, &msg, 0);
                }
            } else {
                break;
            }
            retry_count++;
        }
    }
}