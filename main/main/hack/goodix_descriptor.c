#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GOODIX_DEBUG";
#define I2C_ADDR 0x2c

i2c_master_dev_handle_t dev_handle;

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

uint8_t *report_desc = NULL;
int report_desc_len = 0;

static void hex_dump(const uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
        if ((i & 0x0F) == 0x0F) printf("\n");
    }
    printf("\n");
}

static bool goodix_read_descriptors() {
    esp_err_t ret;
    uint8_t hid_desc[32] = {0};
    uint8_t req[2] = {0x20, 0x00};

    ret = i2c_master_transmit_receive(
        dev_handle, req, 2, hid_desc, 30, pdMS_TO_TICKS(200)
    );
    if (ret != ESP_OK) return false;

    report_desc_len = hid_desc[4] | (hid_desc[5] << 8);
    uint16_t report_reg = hid_desc[6] | (hid_desc[7] << 8);

    if (report_desc_len == 0 || report_desc_len > 1024) return false;

    report_desc = malloc(report_desc_len);
    if (!report_desc) return false;

    uint8_t reg[2] = { report_reg & 0xFF, report_reg >> 8 };
    ret = i2c_master_transmit_receive(
        dev_handle, reg, 2, report_desc, report_desc_len, pdMS_TO_TICKS(500)
    );
    if (ret != ESP_OK) {
        free(report_desc);
        report_desc = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Report Descriptor (%d bytes)", report_desc_len);
    hex_dump(report_desc, report_desc_len);
    return true;
}

void app_main(void) {
    gpio_set_direction(3, GPIO_MODE_OUTPUT);
    gpio_set_level(3, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(3, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 6,
        .sda_io_num = 7,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    goodix_activate_ptp();
    
    goodix_read_descriptors();
}