#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "i2c/elan_i2c.h"
#include "wireless/wireless.h"

#include "freertos/semphr.h"

#define TAG "VBUS_DET"

#define ESPNOW_CHANNEL 1

wireless_msg_t pkt = {0};

uint8_t receiver_mac[] = {0xDC, 0xB4, 0xD9, 0xA3, 0xD4, 0xD4};

TaskHandle_t xHeartbeatTaskHandle = NULL;

#define TAG "VBUS_DET"
static SemaphoreHandle_t vbus_sem = NULL;
uint8_t wireless_mode = 0;

extern bool stop_heartbeat;

static void IRAM_ATTR vbus_det_gpio_isr_handler(void* arg) {
    wireless_mode = gpio_get_level(GPIO_NUM_5);
    xSemaphoreGiveFromISR(vbus_sem, NULL);
    stop_heartbeat = true;
}

void vbus_processor_task(void *pvParameters) {
    pkt.type = VBUS_STATUS;
    while (1) {
        if (xSemaphoreTake(vbus_sem, portMAX_DELAY)) {
            pkt.payload.vbus.vbus_level = wireless_mode;
            esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));
            ESP_LOGI(TAG, "VBUS Changed: %d, Send status: %s", wireless_mode, esp_err_to_name(ret));
        }
    }
}

void vbus_det_init(void) {

    pkt.type = VBUS_STATUS;

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

    vbus_sem = xSemaphoreCreateBinary();
    
    xTaskCreate(vbus_processor_task, "vbus_task", 4096, NULL, 5, NULL);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_5),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_isr_handler_add(GPIO_NUM_5, vbus_det_gpio_isr_handler, NULL);

    wireless_mode = gpio_get_level(GPIO_NUM_5);
    
    pkt.payload.vbus.vbus_level = wireless_mode;
    
    esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));

    if (wireless_mode == 0) {
        xTaskCreate(alive_heartbeat_task, "heartbeat", 2048, NULL, 2, NULL);
        stop_heartbeat = false;
        ESP_LOGI(TAG, "Wireless mode currently, forcing PTP mode activation");
        current_mode = PTP_MODE;
        elan_activate_ptp();
    }
}