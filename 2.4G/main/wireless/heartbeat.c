#include "wireless/wireless.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_now.h"
#include "sdkconfig.h"

uint32_t last_seen_timestamp = 0;

void monitor_link_task(void *arg) {
    while(1) {
        if ((xTaskGetTickCount() - last_seen_timestamp) > pdMS_TO_TICKS(5000)) {
            gpio_set_level(CONFIG_CONN_LED_GPIO_CFG, 1);
        } else {
            gpio_set_level(CONFIG_CONN_LED_GPIO_CFG, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}