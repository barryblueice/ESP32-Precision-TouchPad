#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ELAN_DEBUG";

#define I2C_ADDR         0x2C
#define GPIO_INT         4
#define GPIO_RST         3
#define SCL_IO           6
#define SDA_IO           7

static TaskHandle_t touch_task_handle = NULL;
i2c_master_dev_handle_t dev_handle;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(touch_task_handle, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
}

void parse_touch_data(uint8_t *buf, size_t len) {
    if (len < 32) return;
    uint16_t packet_len = buf[0] | (buf[1] << 8);
    uint8_t report_id = buf[2];

    if (packet_len < 3 || report_id != 0x04) return;

    bool any_finger_active = false;
    char output_line[256] = "";
    char temp[64];

    uint16_t scan_time = buf[28] | (buf[29] << 8);
    uint8_t contact_count = buf[30];
    uint8_t button_state = buf[31];

    printf("Count: %d | Time: %u | Button: %d\n", contact_count, scan_time, button_state);

    for (int i = 0; i < 5; i++) {
        uint8_t *f = &buf[3 + (i * 5)];
        uint8_t status = f[0];

        if (status & 0x01) {
            any_finger_active = true;
            uint16_t x = f[1] | (f[2] << 8);
            uint16_t y = f[3] | (f[4] << 8);
            
            snprintf(temp, sizeof(temp), "[F%d: %4d,%4d] ", i, x, y);
            strcat(output_line, temp);
        }
    }

    if (any_finger_active) {
        printf("Active: %s\n", output_line);
    } else {
        printf("No Fingers Pressed (All Released)\n");
    }
}

void touch_task(void *pvParameters) {
    uint8_t rx_buf[64];
    
    while (1) {
        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10))) {
            while (gpio_get_level(GPIO_INT) == 0) {
                esp_err_t ret = i2c_master_receive(dev_handle, rx_buf, sizeof(rx_buf), 20);
                if (ret == ESP_OK) {
                    parse_touch_data(rx_buf, sizeof(rx_buf));
                }
                vTaskDelay(pdMS_TO_TICKS(1)); 
            }
        }
    }
}

void init_all() {
    gpio_set_direction(GPIO_RST, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(GPIO_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(200));

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_IO,
        .sda_io_num = SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    gpio_config_t int_cfg = {
        .pin_bit_mask = (1ULL << GPIO_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&int_cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_INT, gpio_isr_handler, NULL);
}

void app_main(void) {
    xTaskCreate(touch_task, "touch_task", 4096, NULL, 10, &touch_task_handle);
    init_all();

    uint8_t ptp_cmd[] = {0x05, 0x00, 0x33, 0x03, 0x06, 0x00, 0x05, 0x00, 0x03, 0x03, 0x00};
    i2c_master_transmit(dev_handle, ptp_cmd, sizeof(ptp_cmd), 100);

    ESP_LOGI(TAG, "Debug Start - Watch for Raw Data...");
}