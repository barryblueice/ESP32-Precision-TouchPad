#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ELAN_PTP_FINAL";

#define I2C_ADDR    0x15
#define SCL_IO      9
#define SDA_IO      8
#define RST_IO      6
#define INT_IO      7

i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;
uint8_t *report_desc = NULL;
int report_desc_len = 0;

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
typedef struct {
    uint16_t max_x;
    uint16_t max_y;
} tp_max_xy_t;

static tp_max_xy_t elan_parse_max_xy(const uint8_t *desc, int len)
{
    tp_max_xy_t max_val = {0,0};

    for (int i = 0; i < len-1; i++) {
        // X 坐标逻辑最大值
        if (desc[i] == 0x09 && desc[i+1] == 0x30) { // USAGE X
            for (int j = i; j < len-2; j++) {
                if (desc[j] == 0x26) { // LOGICAL_MAXIMUM (2 byte)
                    max_val.max_x = desc[j+1] | (desc[j+2]<<8);
                    ESP_LOGI(TAG, "Raw X LOGICAL_MAX: 0x%02X 0x%02X -> %d", desc[j+1], desc[j+2], max_val.max_x);
                    break;
                }
            }
        }
        // Y 坐标逻辑最大值
        if (desc[i] == 0x09 && desc[i+1] == 0x31) { // USAGE Y
            for (int j = i; j < len-2; j++) {
                if (desc[j] == 0x26) { // LOGICAL_MAXIMUM (2 byte)
                    max_val.max_y = desc[j+1] | (desc[j+2]<<8);
                    ESP_LOGI(TAG, "Raw Y LOGICAL_MAX: 0x%02X 0x%02X -> %d", desc[j+1], desc[j+2], max_val.max_y);
                    break;
                }
            }
        }
    }

    ESP_LOGI(TAG, "Detected Max X = %d, Max Y = %d", max_val.max_x, max_val.max_y);
    return max_val;
}


static void hex_dump(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
        if ((i & 0x0F) == 0x0F) printf("\n");
    }
    printf("\n");
}

esp_err_t elan_activate_ptp_payload() {
    uint8_t payload[] = {
        0x05, 0x00, 0x33, 0x03, 0x06, 0x00, 0x05, 0x00, 0x03, 0x03, 0x00
    };
    ESP_LOGI(TAG, "Sending Specialized PTP Activation Payload...");
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

static bool elan_read_descriptors() {
    esp_err_t ret;
    uint8_t hid_desc[32] = {0};
    uint8_t hid_req[2] = {0x01, 0x00};

    ret = i2c_master_transmit_receive(dev_handle, hid_req, 2, hid_desc, 30, pdMS_TO_TICKS(200));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HID Descriptor read failed");
        return false;
    }

    hex_dump(hid_desc, 30);
    report_desc_len = hid_desc[4] | (hid_desc[5]<<8);
    uint16_t report_reg = hid_desc[6] | (hid_desc[7]<<8);

    if (report_desc_len == 0 || report_desc_len > 1024) return false;

    report_desc = malloc(report_desc_len);
    if (!report_desc) return false;

    uint8_t reg[2] = { report_reg & 0xFF, report_reg >> 8 };
    ret = i2c_master_transmit_receive(dev_handle, reg, 2, report_desc, report_desc_len, pdMS_TO_TICKS(500));
    if (ret != ESP_OK) {
        free(report_desc);
        report_desc = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Report Descriptor (%d bytes):", report_desc_len);
    hex_dump(report_desc, report_desc_len);
    return true;
}

static finger_layout_t parse_finger_layout(const uint8_t *desc, int len)
{
    finger_layout_t layout = {0};

    for (int i = 0; i < len-1; i++) {
        if (desc[i] == 0x09 && desc[i+1] == 0x30) { // X usage
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

static void parse_ptp_report(uint8_t *data, int len, finger_layout_t layout)
{
    uint16_t total_len = data[0] | (data[1]<<8);
    uint8_t report_id = data[2];
    if (report_id != 0x04 || total_len < 3) return;

    uint8_t *p = data + 3;
    int finger_count = (total_len - 3) / layout.block_size;

    for (int i = 0; i < finger_count; i++) {
        bool tip = p[layout.tip_offset] & 0x01;
        uint8_t fid = p[layout.id_offset];
        uint16_t x=0, y=0;

        if (layout.nibble_packed) {
            uint8_t b4 = p[layout.x_offset];
            uint8_t b5 = p[layout.x_offset+1];
            uint8_t b6 = p[layout.y_offset];
            x = (b4 << 4) | ((b5 & 0xF0)>>4);
            y = (b6 << 4) | (b5 & 0x0F);
        } else {
            x = p[layout.x_offset] | (p[layout.x_offset+1]<<8);
            y = p[layout.y_offset] | (p[layout.y_offset+1]<<8);
        }

        if (tip)
            printf("PTP TOUCH [ID:%d] X=%d Y=%d\n", fid, x, y);
        else
            printf("PTP RELEASE [ID:%d]\n", fid);

        p += layout.block_size;
    }
}

void elan_init_sequence() {
    uint8_t rx[128];
    gpio_set_direction(RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    uint8_t read_desc[] = {0x01, 0x00};
    i2c_master_transmit_receive(dev_handle, read_desc, 2, rx, 30, 100);
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t pwr_on[] = {0x05,0x00,0x08,0x00};
    i2c_master_transmit(dev_handle, pwr_on, 4, 100);
    vTaskDelay(pdMS_TO_TICKS(20));

    elan_activate_ptp_payload();
    vTaskDelay(pdMS_TO_TICKS(100));

    uint8_t get_mode[] = {0x05,0x00,0x13,0x03};
    if (i2c_master_transmit_receive(dev_handle, get_mode, 4, rx, 6, 100) == ESP_OK) {
        ESP_LOGI(TAG, "Mode Register Status: 0x%02x", rx[4]);
    }

    if (!elan_read_descriptors()) {
        ESP_LOGE(TAG, "Failed to read report descriptor");
        return;
    }
    tp_max_xy_t max_xy = elan_parse_max_xy(report_desc, report_desc_len);
}

void elan_i2c_task(void *arg) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_IO,
        .sda_io_num = SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    gpio_set_direction(INT_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INT_IO, GPIO_PULLUP_ONLY);

    elan_init_sequence();

    finger_layout_t layout = parse_finger_layout(report_desc, report_desc_len);

    uint8_t data[64];
    while(1) {
        if (gpio_get_level(INT_IO) == 0) {
            esp_err_t ret = i2c_master_receive(dev_handle, data, sizeof(data), pdMS_TO_TICKS(10));
            if (ret == ESP_OK) {
                parse_ptp_report(data, sizeof(data), layout);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

void app_main() {
    xTaskCreate(elan_i2c_task, "elan_i2c_task", 8192, NULL, 10, NULL);
}
