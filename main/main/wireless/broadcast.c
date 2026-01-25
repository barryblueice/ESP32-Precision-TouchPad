#include "wireless/wireless.h"
#include "i2c/elan_i2c.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_log.h"
#include <stdint.h>

static const char *TAG = "BROADCAST";

static uint8_t last_ptp_input_mode = 0xFF;

void wifi_now_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len == 1) {
        uint8_t received_cmd = data[0];

        if (received_cmd != last_ptp_input_mode) {

            if (received_cmd == PTP_MODE) {
                ESP_LOGI(TAG, "Wireless Mode 0x03 detected: Activating ELAN PTP");
                current_mode = PTP_MODE;
                elan_activate_ptp();
            } 
            else if (received_cmd == MOUSE_MODE) {
                ESP_LOGI(TAG, "Wireless Mode 0x01 detected: Activating ELAN Mouse");
                current_mode = MOUSE_MODE;
                elan_activate_mouse();
            }
        }
    }
}

void wireless_init() {
    esp_now_register_recv_cb(wifi_now_recv_cb);
}