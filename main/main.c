#include <stdio.h>
#include "esp_log.h"
#include "usb/usbhid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "i2c/elan_i2c.h"

void app_main(void) {

    elan_i2c_init();
    tp_interrupt_init();
    
    tp_queue = xQueueCreate(1, sizeof(tp_multi_msg_t));

    usbhid_init();

    xTaskCreate(elan_i2c_task, "elan_i2c", 4096, NULL, 10, NULL);
    xTaskCreate(usb_mount_task, "mode_sel", 4096, NULL, 11, NULL);
    xTaskCreate(usbhid_task, "hid", 4096, NULL, 12, NULL);

    while (1) {
        tud_task(); 
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}