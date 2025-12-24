#include <stdio.h>
#include "esp_log.h"
#include "usb/usbhid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"

static const char *TAG = "ESP32";

void app_main(void)
{
    usbhid_init();

    xTaskCreate(usbhid_task, "hid", 4096, NULL, 5, NULL);

    while (1) {
        tud_task();          // 必须运行
        vTaskDelay(1);       // 不会阻塞
    }
}