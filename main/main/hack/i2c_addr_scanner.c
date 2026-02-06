#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "I2C_SCANNER";

#define I2C_MASTER_SCL_IO           GPIO_NUM_6      
#define I2C_MASTER_SDA_IO           GPIO_NUM_7      
#define I2C_MASTER_PORT             I2C_NUM_0

void app_main(void)
{
    ESP_LOGI(TAG, "开始扫描 I2C 总线...");

    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_PORT,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    printf("00:         ");

    for (uint8_t i = 3; i < 0x7F; i++) {
        if (i % 16 == 0) {
            printf("\n%.2x:", i);
        }

        esp_err_t ret = i2c_master_probe(bus_handle, i, 100);

        if (ret == ESP_OK) {
            printf(" %.2x", i);
        } else {
            printf(" --");
        }
    }
    printf("\n\n扫描完成。\n");
}