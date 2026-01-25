#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_now.h"

#include "freertos/semphr.h"

#define TAG "VBUS_DET"

#define ESPNOW_CHANNEL 1

typedef struct {
    uint32_t wireless_state;
    char message[16];
} __attribute__((packed)) wireless_packet_t;

wireless_packet_t pkt;

uint8_t receiver_mac[] = {0xDC, 0xB4, 0xD9, 0xA3, 0xD4, 0xD4};

#define TAG "VBUS_DET"
static SemaphoreHandle_t vbus_sem = NULL;
static uint8_t current_level = 0;

static void IRAM_ATTR vbus_det_gpio_isr_handler(void* arg) {
    current_level = gpio_get_level(GPIO_NUM_5);
    xSemaphoreGiveFromISR(vbus_sem, NULL);
}

void vbus_processor_task(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(vbus_sem, portMAX_DELAY)) {
            pkt.wireless_state = current_level;
            esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));
            ESP_LOGI(TAG, "VBUS Changed: %d, Send status: %s", current_level, esp_err_to_name(ret));
        }
    }
}

void vbus_det_init(void) {

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

    current_level = gpio_get_level(GPIO_NUM_5);
    
    esp_err_t ret = esp_now_send(receiver_mac, (uint8_t *)&pkt, sizeof(pkt));
}