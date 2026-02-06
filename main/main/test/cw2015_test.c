#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"

static const char *TAG = "CW2015_APP";

#define I2C_MASTER_SCL_IO           34
#define I2C_MASTER_SDA_IO           33
#define CW2015_ADDR                 0x62

#define REG_VCELL_H                 0x02
#define REG_SOC_H                   0x04
#define REG_MODE                    0x0A
#define REG_CONFIG                  0x08
#define REG_BATINFO_START           0x10
#define SIZE_BATINFO                64

i2c_master_dev_handle_t dev_handle;

static const uint8_t cw_bat_config_info[SIZE_BATINFO] = {
    0x15, 0x7E, 0x7C, 0x5C, 0x64, 0x6A, 0x65, 0x5C, 
    0x55, 0x53, 0x56, 0x61, 0x6F, 0x66, 0x50, 0x48,
    0x43, 0x42, 0x40, 0x43, 0x4B, 0x5F, 0x75, 0x7D,
    0x52, 0x44, 0x07, 0xAE, 0x11, 0x22, 0x40, 0x56,
    0x6C, 0x7C, 0x85, 0x86, 0x3D, 0x19, 0x8D, 0x1B,
    0x06, 0x34, 0x46, 0x79, 0x8D, 0x90, 0x90, 0x46,
    0x67, 0x80, 0x97, 0xAF, 0x80, 0x9F, 0xAE, 0xCB,
    0x2F, 0x00, 0x64, 0xA5, 0xB5, 0x11, 0xD0, 0x11
};

esp_err_t cw2015_read_reg(uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, pdMS_TO_TICKS(1000));
}

esp_err_t cw2015_write_reg(uint8_t reg_addr, uint8_t val) {
    uint8_t write_buf[2] = {reg_addr, val};
    return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), pdMS_TO_TICKS(1000));
}

void i2c_bus_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CW2015_ADDR,
        .scl_speed_hz = 100000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    ESP_LOGI(TAG, "I2C initialized on GPIO %d(SDA), %d(SCL)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
}

void cw2015_update_config(void) {
    uint8_t read_back[SIZE_BATINFO];
    
    ESP_LOGI(TAG, "Checking and updating battery profile...");

    cw2015_write_reg(REG_MODE, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    for (int i = 0; i < SIZE_BATINFO; i++) {
        cw2015_write_reg(REG_BATINFO_START + i, cw_bat_config_info[i]);
    }

    for (int i = 0; i < SIZE_BATINFO; i++) {
        cw2015_read_reg(REG_BATINFO_START + i, &read_back[i], 1);
    }

    if (memcmp(cw_bat_config_info, read_back, SIZE_BATINFO) == 0) {
        ESP_LOGI(TAG, "Profile write verified successfully.");
        cw2015_write_reg(REG_CONFIG, 0x01);
    } else {
        ESP_LOGE(TAG, "Profile verification failed! Check I2C wiring.");
    }
}

void app_main(void) {
    i2c_bus_init();

    cw2015_update_config();

    while (1) {
        uint8_t soc = 0;
        uint8_t v_bytes[2];

        if (cw2015_read_reg(REG_SOC_H, &soc, 1) == ESP_OK) {
            if (soc <= 100) {
                ESP_LOGI(TAG, "Battery SOC: %d%%", soc);
            } else {
                ESP_LOGW(TAG, "Battery SOC Initializing...");
            }
        }

        if (cw2015_read_reg(REG_VCELL_H, v_bytes, 2) == ESP_OK) {
            uint16_t vcell = (v_bytes[0] << 8) | v_bytes[1];
            float voltage = (float)vcell * 0.305 / 1000.0;
            ESP_LOGI(TAG, "Battery Voltage: %.3f V", voltage);
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}