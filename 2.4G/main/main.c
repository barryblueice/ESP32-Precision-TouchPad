#include <stdio.h>
#include "esp_log.h"
#include "usb/usbhid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "nvs/ptp_nvs.h"

#include "wireless/wireless.h"

void app_main(void) {

    ESP_ERROR_CHECK(nvs_mode_init());
    
    current_mode = MOUSE_MODE;
    
    tp_queue = xQueueCreate(1, sizeof(ptp_report_t));
    mouse_queue = xQueueCreate(1, sizeof(mouse_hid_report_t));

    main_queue_set = xQueueCreateSet(1 + 1);
    xQueueAddToSet(mouse_queue, main_queue_set);
    xQueueAddToSet(tp_queue, main_queue_set);
    
    xTaskCreate(usb_mount_task, "mode_sel", 4096, NULL, 11, NULL);

    usbhid_init();
    wifi_recieve_task_init();

    xTaskCreate(usbhid_task, "hid", 4096, NULL, 12, NULL);

    xTaskCreate(monitor_link_task, "heartbeat", 2048, NULL, 2, NULL);

    broadcast_init();

    while (1) {
        tud_task(); 
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}