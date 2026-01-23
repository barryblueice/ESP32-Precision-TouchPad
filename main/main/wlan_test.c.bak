#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_chip_info.h"

static const char *TAG = "WIFI_TEST";

void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void wifi_scan_performance() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "开始 Wi-Fi 扫描...");

    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = true
    };

    while (1) {
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

        uint16_t ap_count = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
        
        wifi_ap_record_t *ap_records = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * ap_count);
        if (ap_records == NULL) {
            ESP_LOGE(TAG, "内存分配失败");
            return;
        }

        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

        ESP_LOGI(TAG, "--------------------------------------------------");
        ESP_LOGI(TAG, "发现 %d 个网络:", ap_count);
        ESP_LOGI(TAG, "%-32s | %-4s | %-4s", "SSID", "信道", "RSSI");

        for (int i = 0; i < ap_count; i++) {
            ESP_LOGI(TAG, "%-32s | %-4d | %-4d dBm", 
                     (char *)ap_records[i].ssid, 
                     ap_records[i].primary, 
                     ap_records[i].rssi);
        }
        ESP_LOGI(TAG, "--------------------------------------------------");

        free(ap_records);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void app_main(void) {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("正在测试 ESP32-S2 无线能力...\n");
    printf("芯片核心数: %d\n", chip_info.cores);
    printf("硅版本: %d\n", chip_info.revision);

    init_nvs();
    wifi_scan_performance();
}