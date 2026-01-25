#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "class/hid/hid_device.h"

#include "esp_wifi.h"
#include "esp_now.h"

#include "esp_timer.h"

#include "math.h"

#include "i2c/elan_i2c.h"

#include "usb/usbhid.h"

#include "wireless/wireless.h"

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
    .idProduct          = 0x072A,
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
    "R-SODIUM Precision TouchPad",               // 2: Product
    "0D00072A00000000",                          // 3: Serial Number
    "Precision Touchpad HID Interface"           // 4: HID Interface
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

static uint8_t last_ptp_input_mode = 0xFF;

void usb_mount_task(void *arg) {
    while (1) {
        if (tud_mounted()) {
            if (ptp_input_mode != last_ptp_input_mode) {
                switch (ptp_input_mode) {
                case 0x03:
                    ESP_LOGI(TAG, "Mode 0x03 detected: Activating ELAN PTP");
                    current_mode = PTP_MODE;
                    elan_activate_ptp();
                    // ESP_LOGI(TAG, "Mode 0x01 detected: Activating ELAN MOUSE");
                    // current_mode = MOUSE_MODE;
                    // elan_activate_mouse();
                    break;
                case 0x00:
                    if (wireless_mode == 1) {
                        ESP_LOGI(TAG, "Mode 0x01 detected: Activating ELAN MOUSE");
                        current_mode = MOUSE_MODE;
                        elan_activate_mouse();
                        break;
                    }
                default:
                    break;
                }
                last_ptp_input_mode = ptp_input_mode;
            }
        } else {
            ptp_input_mode = 0x00;
        }
        vTaskDelay(100);
    }
}

void usbhid_task(void *arg) {
    tp_multi_msg_t msg;
    mouse_msg_t mouse_msg;
    static uint16_t last_scan_time = 0;
    wireless_msg_t pkt = {0};

    while (1) {

        QueueSetMemberHandle_t xActivatedMember = xQueueSelectFromSet(main_queue_set, portMAX_DELAY);

        if (xActivatedMember == mouse_queue) {
            if (xQueueReceive(mouse_queue, &mouse_msg, portMAX_DELAY)) {

                mouse_hid_report_t report = {0};

                int move_x = (int)(mouse_msg.x * SENSITIVITY);
                int move_y = (int)(mouse_msg.y * SENSITIVITY);

                if (move_x > 127)  move_x = 127;
                if (move_x < -127) move_x = -127;

                if (move_y > 127)  move_y = 127;
                if (move_y < -127) move_y = -127;

                report.x = (int8_t)move_x;
                report.y = (int8_t)move_y;

                report.buttons = mouse_msg.buttons & 0x07;

                // ESP_LOGI(TAG, "X: %d, y:%d", report.x, report.y);

                if (wireless_mode == 1) {
                    if (tud_hid_n_ready(1)) {
                        tud_hid_n_report(1, REPORTID_MOUSE, &report, sizeof(report));
                    } 
                } else {
                    pkt.type = MOUSE_MODE;
                    pkt.payload.mouse = report;
                    esp_now_send(receiver_mac, (uint8_t*)&pkt, sizeof(pkt));
                }
            }
        } else if (xActivatedMember == tp_queue) {
            if (xQueueReceive(tp_queue, &msg, portMAX_DELAY)) {

                ptp_report_t report = {0};

                uint32_t now = esp_timer_get_time() / 100;
                if (now <= last_scan_time) {
                    now = last_scan_time + 1;
                }
                last_scan_time = now;
                report.scan_time = (uint16_t)now;
                
                for (int i = 0; i < 5; i++) {
                    if (msg.fingers[i].tip_switch) {
                        // uint16_t tx = (msg.fingers[i].x * HID_MAX + RAW_X_MAX / 2) / RAW_X_MAX;
                        // uint16_t ty = (msg.fingers[i].y * HID_MAX + RAW_Y_MAX / 2) / RAW_Y_MAX;
                        uint16_t tx = msg.fingers[i].x;
                        uint16_t ty = msg.fingers[i].y;
                        // if (tx > HID_MAX) tx = HID_MAX;
                        // if (ty > HID_MAX) ty = HID_MAX;
                        uint8_t contact_id = i; 

                        report.fingers[i].tip_conf_id = PTP_CONFIDENCE_BIT | PTP_TIP_SWITCH_BIT | (contact_id << 2);
                        report.fingers[i].x = tx;
                        report.fingers[i].y = ty;
                    } else {
                        report.fingers[i].tip_conf_id = (i << 2);
                        report.fingers[i].x = 0;
                        report.fingers[i].y = 0;
                    }
                }

                report.contact_count = msg.actual_count;

                report.buttons = (msg.button_mask > 0) ? 0x01 : 0x00;

                if (wireless_mode == 1) {
                    if (tud_hid_n_ready(0)) {
                        tud_hid_n_report(0, REPORTID_TOUCHPAD, &report, sizeof(report));
                    }
                } else {
                    pkt.type = PTP_MODE;
                    pkt.payload.ptp = report;
                    esp_now_send(receiver_mac, (uint8_t*)&pkt, sizeof(pkt));
                }
            }
        }
    }
}
