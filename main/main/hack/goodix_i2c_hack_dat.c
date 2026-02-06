#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "GOODIX_DEBUG";
#define I2C_ADDR 0x2c

i2c_master_dev_handle_t dev_handle;

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

    elan_activate_ptp();
    
    ESP_LOGI(TAG, "Attempting to read HID Descriptor...");
    uint8_t desc_req[] = {0x01, 0x00};
    uint8_t buffer[32];
    
    i2c_master_transmit_receive(dev_handle, desc_req, 2, buffer, 30, 100);
    
    ESP_LOGI(TAG, "Descriptor Data: %02x %02x %02x...", buffer[0], buffer[1], buffer[2]);

    while (1) {
        uint8_t data[32] = {0};
        if (gpio_get_level(4) == 0) {
            esp_err_t ret = i2c_master_receive(dev_handle, data, 32, 100);
            if (ret == ESP_OK) {
                printf("Raw Data: ");
                for(int i=0; i<32; i++) printf("%02x ", data[i]);
                printf("\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}