#include <stdio.h>
#include "esp_log.h"
#include "usb/usbhid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "nvs/ptp_nvs.h"
#include "wireless/wireless.h"

#include "sdkconfig.h"

#if CONFIG_ELAN_LENOVO_33370A
    #include "i2c/ELAN/elan_i2c.h"
#endif

#include "i2c/I2C_HID_Report.h"

void app_main(void) {

    elan_i2c_init();
    tp_interrupt_init();

    ESP_ERROR_CHECK(nvs_mode_init());
    
    current_mode = MOUSE_MODE;
    
    tp_queue = xQueueCreate(1, sizeof(tp_multi_msg_t));
    mouse_queue = xQueueCreate(1, sizeof(mouse_msg_t));

    main_queue_set = xQueueCreateSet(1 + 1);
    xQueueAddToSet(mouse_queue, main_queue_set);
    xQueueAddToSet(tp_queue, main_queue_set);
    
    xTaskCreate(usb_mount_task, "mode_sel", 4096, NULL, 11, NULL);
    
    vbus_det_init();

    usbhid_init();

    #ifdef CONFIG_ELAN_LENOVO_33370A
        xTaskCreate(elan_i2c_task, "elan_i2c", 4096, NULL, 10, NULL);
        
    #endif

    xTaskCreate(usbhid_task, "hid", 4096, NULL, 12, NULL);

    while (1) {
        tud_task(); 
        vTaskDelay(pdMS_TO_TICKS(1)); 
    }
}