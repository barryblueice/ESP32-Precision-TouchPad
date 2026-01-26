#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "class/hid/hid_device.h"

#include "esp_timer.h"

#include "math.h"

#include "wireless/wireless.h"

#include "usb/usbhid.h"

#include "esp_now.h"

#define TPD_REPORT_SIZE   6

static const char *TAG = "USB_HID_TP";

#define REPORTID_TOUCHPAD         0x01
#define REPORTID_MOUSE            0x02
#define REPORTID_MAX_COUNT        0x03
#define REPORTID_PTPHQA           0x04
#define REPORTID_FEATURE          0x05
#define REPORTID_FUNCTION_SWITCH  0x06

#define TPD_REPORT_ID 0x01
#define TPD_REPORT_SIZE_WITHOUT_ID (sizeof(touchpad_report_t) - 1)

const float SENSITIVITY = 3.0f;

// USB Device Descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x0D00,
    .idProduct          = 0x072B,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// String Descriptors
char const* string_desc[] = {
    (const char[]){0x09, 0x04},                  // 0: Language
    "R-SODIUM Technology",                       // 1: Manufacturer
    "R-SODIUM PTP 2.4G Reciever",                // 2: Product
    "0D00072B00000000",                          // 3: Serial Number
    "PTP Wireless HID Interface"                 // 4: HID Interface
};

// TinyUSB callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    return (instance == 0) ? ptp_hid_report_descriptor : mouse_hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == REPORTID_FEATURE) {
            buffer[0] = 0x03;
            return 1;
        }
        if (report_id == REPORTID_MAX_COUNT) {
            buffer[0] = 0x15; 
            return 1;
        }
        if (report_id == REPORTID_PTPHQA) {
            memset(buffer, 0, 256);
            return 256; 
        }
    }
    return 0;
}

static uint8_t ptp_input_mode = 0x00;

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORTID_FEATURE) {
        if (bufsize >= 1) {
            ptp_input_mode = buffer[0];
        }
    }
}

void usbhid_init(void) {
    tinyusb_config_t tusb_cfg = TINYUSB_DEFAULT_CONFIG();
    tusb_cfg.descriptor.device = &desc_device;
    tusb_cfg.descriptor.full_speed_config = desc_configuration;
    tusb_cfg.descriptor.string = string_desc;
    tusb_cfg.descriptor.string_count = sizeof(string_desc)/sizeof(string_desc[0]);

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}

#define PTP_CONFIDENCE_BIT (1 << 0)
#define PTP_TIP_SWITCH_BIT (1 << 1)

// #define RAW_X_MAX 3679
// #define RAW_Y_MAX 2261
// #define HID_MAX   4095

static uint8_t last_ptp_input_mode = 0xFF;

void usb_mount_task(void *arg) {
    while (1) {
        if (tud_mounted()) {
            if (ptp_input_mode != last_ptp_input_mode) {
                switch (ptp_input_mode) {
                case 0x03:
                    ESP_LOGI(TAG, "Mode 0x03 detected: Activating ELAN PTP");
                    current_mode = PTP_MODE;
                    // elan_activate_ptp();
                    // ESP_LOGI(TAG, "Mode 0x01 detected: Activating ELAN MOUSE");
                    // current_mode = MOUSE_MODE;
                    // elan_activate_mouse();
                    break;
                case 0x00:
                    ESP_LOGI(TAG, "Mode 0x01 detected: Activating ELAN MOUSE");
                    current_mode = MOUSE_MODE;
                    // elan_activate_mouse();
                    break;
                default:
                    break;
                }
                esp_now_send(broadcast_mac, (const uint8_t *)&current_mode, 1);
                last_ptp_input_mode = ptp_input_mode;
            }
        } else {
            ptp_input_mode = 0x00;
        }
        vTaskDelay(100);
    }
}

void usbhid_task(void *arg) {
    ptp_report_t tp_report; 
    mouse_hid_report_t mouse_report;

    while (1) {
        QueueSetMemberHandle_t xActivatedMember = xQueueSelectFromSet(main_queue_set, portMAX_DELAY);

        if (xActivatedMember == mouse_queue) {
            if (xQueueReceive(mouse_queue, &mouse_report, 0)) {
                if (tud_hid_n_ready(1)) {
                    tud_hid_n_report(1, REPORTID_MOUSE, &mouse_report, sizeof(mouse_report));
                }
            }
        } 
        else if (xActivatedMember == tp_queue) {
            if (xQueueReceive(tp_queue, &tp_report, 0)) {
                if (tud_hid_n_ready(0)) {
                    tud_hid_n_report(0, REPORTID_TOUCHPAD, &tp_report, sizeof(tp_report));
                }
            }
        }
    }
}
