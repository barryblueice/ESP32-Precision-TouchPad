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

#include "i2c/elan_i2c.h"

#include "usb/usbhid.h"

#define MY_REPORT_ID      0x01
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
    .idProduct          = 0x072A,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

// String Descriptors
char const* string_desc[] = {
    (const char[]){0x09, 0x04},                  // 0: Language
    "R-SODIUM Technology",                       // 1: Manufacturer
    "R-SODIUM Precision TouchPad",               // 2: Product
    "0D00072A00000000",                          // 3: Serial Number
    "Precision Touchpad HID Interface"           // 4: HID Interface
};

// TinyUSB callbacks
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report_descriptor;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) {
    if (report_type == HID_REPORT_TYPE_FEATURE) {
        if (report_id == REPORTID_FEATURE) {
            buffer[0] = 0x03;
            return 1;
        }
        if (report_id == REPORTID_MAX_COUNT) {
            buffer[0] = 0x05; 
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

#if TUD_OPT_HIGH_SPEED
    tusb_cfg.descriptor.high_speed_config = desc_configuration;
#endif

    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));
}

typedef struct __attribute__((packed)) {
    uint8_t tip_conf_id;  // Bit0:Conf, Bit1:Tip, Bit2-7:ID
    uint16_t x;           
    uint16_t y;           
} finger_t;

typedef struct __attribute__((packed)) {
    finger_t fingers[5];   // 5 * 5 = 25 bytes
    uint16_t scan_time;    // 2 bytes
    uint8_t contact_count; // 1 byte
    uint8_t buttons;       // 1 byte
} ptp_report_t;

void usbhid_task(void *arg) {
    tp_multi_msg_t msg;
    uint16_t sc_time = 0;
    
    while (1) {
        if (xQueueReceive(tp_queue, &msg, portMAX_DELAY)) {
            ptp_report_t report = {0};
            
            if (msg.actual_count > 0) {
                report.fingers[0].tip_conf_id = 0x03 | (0 << 2); 
                report.fingers[0].x = (uint16_t)(msg.fingers[0].x * 4095 / 3679);
                report.fingers[0].y = (uint16_t)(msg.fingers[0].y * 4095 / 2261);
                report.contact_count = 1;
            } else {
                report.fingers[0].tip_conf_id = 0x01;
                report.contact_count = 0;
            }

            sc_time += 100;
            report.scan_time = sc_time;

            // 调用 TinyUSB 发送
            tud_hid_report(REPORTID_TOUCHPAD, &report, sizeof(report));
        }
    }
}