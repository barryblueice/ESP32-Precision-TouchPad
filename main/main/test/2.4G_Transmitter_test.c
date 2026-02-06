#include <stdint.h>

typedef struct {
    uint32_t send_count;
    char message[16];
} __attribute__((packed)) test_packet_t;

#define ESPNOW_CHANNEL 1

#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "Transmitter";

// dc:b4:d9:a3:d4:d4

uint8_t receiver_mac[] = {0xDC, 0xB4, 0xD9, 0xA3, 0xD4, 0xD4};

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_now_init());

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, receiver_mac, 6);
    peer.channel = ESPNOW_CHANNEL;
    peer.encrypt = false;
    ESP_ERROR_CHECK(esp_now_add_peer(&peer));

    test_packet_t pkt;
    pkt.send_count = 0;
    strcpy(pkt.message, "RSSI_TEST");

    while (1) {
        pkt.send_count++;
        esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));
        // 日志通过原生 USB 打印
        ESP_LOGI(TAG, "Sending Packet #%lu, Status: %s", pkt.send_count, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}