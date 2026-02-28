#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "i2c/I2C_HID_Report.h"

#if CONFIG_ELAN_LENOVO_33370A
    #include "i2c/ELAN/elan_i2c.h"
#elif CONFIG_MI_GOODIX_HAPTIC_ENGINE
    #include "i2c/goodix/goodix_i2c.h"
#endif

static const char *TAG = "WATCHDOG";

esp_timer_handle_t timeout_watchdog_timer;

void watchdog_timeout_callback(void* arg) {

    if (current_mode == PTP_MODE && global_watchdog_start) {

        tp_multi_msg_t release_msg = {0};

        global_scan_time += 1000;
        release_msg.scan_time = global_scan_time;

        release_msg.fingers[0].contact_id = 0;
        release_msg.fingers[0].tip_switch = 0;
        
        release_msg.fingers[0].x = 0;
        release_msg.fingers[0].y = 0;

        release_msg.actual_count = 1;
        release_msg.button_mask = 0;

        if (tp_queue != NULL) {
            xQueueOverwrite(tp_queue, &release_msg);
            // ESP_DRAM_LOGW(TAG, "Watchdog triggered: Force simulating finger release for ID 0");
        }
    }

}