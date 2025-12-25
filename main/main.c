#include <stdio.h>
#include "esp_log.h"
#include "usb/usbhid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "i2c/elan_i2c.h"

void app_main(void) {

    tp_queue = xQueueCreate(5, sizeof(tp_multi_msg_t));

    usbhid_init();

    xTaskCreate(elan_i2c_task, "elan_i2c", 4096, NULL, 10, NULL);
    xTaskCreate(usbhid_task, "hid", 4096, NULL, 5, NULL);

    while (1) {
        tud_task();
        vTaskDelay(1);
    }
}