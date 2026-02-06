#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ELAN_DEBUG";
#define I2C_ADDR 0x15

void app_main(void) {
    // --- 1. 硬件强置复位 ---
    gpio_set_direction(6, GPIO_MODE_OUTPUT);
    gpio_set_level(6, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(6, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    // --- 2. I2C 初始化 ---
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 9,
        .sda_io_num = 8,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // --- 3. 尝试唤醒触摸板 (关键步骤) ---
    // 写入命令 0x0005 (Power Register) 值为 0 (Power On)
    // 根据 HID over I2C 标准，通常是 2字节地址 + 数据
    uint8_t power_on[] = {0x05, 0x00, 0x00, 0x00}; 
    i2c_master_transmit(dev_handle, power_on, sizeof(power_on), 100);
    
    ESP_LOGI(TAG, "Attempting to read HID Descriptor...");
    uint8_t desc_req[] = {0x01, 0x00}; // 读取描述符请求
    uint8_t buffer[32];
    
    // 尝试先写再读 (Transmit then Receive)
    i2c_master_transmit_receive(dev_handle, desc_req, 2, buffer, 30, 100);
    
    ESP_LOGI(TAG, "Descriptor Data: %02x %02x %02x...", buffer[0], buffer[1], buffer[2]);

    // --- 4. 循环轮询 (如果不触发中断，我们主动读) ---
    while (1) {
        uint8_t data[32] = {0};
        // 检查 INT 引脚 (GPIO 7) 是否为低
        if (gpio_get_level(7) == 0) {
            // 如果中断引脚被触摸板拉低，尝试读取 32 字节数据
            esp_err_t ret = i2c_master_receive(dev_handle, data, 32, 100);
            if (ret == ESP_OK) {
                printf("Raw Data: ");
                for(int i=0; i<32; i++) printf("%02x ", data[i]);
                printf("\n");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz 轮询
    }
}