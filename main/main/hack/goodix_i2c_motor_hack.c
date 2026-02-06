#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "HAPTIC_FIX";

#define MOTOR_ADDR       0x50
#define PIN_SDA          7
#define PIN_SCL          6
#define PIN_RST          3

i2c_master_dev_handle_t motor_handle;

void app_main(void) {
    gpio_set_direction(PIN_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(500));

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PIN_SCL,
        .sda_io_num = PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t motor_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MOTOR_ADDR,
        .scl_speed_hz = 100000, 
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &motor_cfg, &motor_handle));

    while (1) {
        uint8_t cmd[] = {0x01, 0x81};
        
        i2c_master_transmit(motor_handle, cmd, sizeof(cmd), 50);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}