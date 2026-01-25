#include "i2c/elan_i2c.h"
#include "wireless/wireless.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"

bool stop_heartbeat = false;

void alive_heartbeat_task(void *pvParameters) {
    wireless_msg_t alive_pkt;
    alive_pkt.type = ALIVE_MODE;
    
    while (1) {
        if (stop_heartbeat) {
            break;
        }

        // ESP_LOGW("HeartBeat", "Starting Alive Heartbeat Task");

        alive_pkt.payload.alive.battery_level = 100;
        alive_pkt.payload.alive.uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
        alive_pkt.payload.alive.vbus_level = wireless_mode;

        esp_now_send(receiver_mac, (uint8_t*)&alive_pkt, sizeof(alive_pkt));

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}