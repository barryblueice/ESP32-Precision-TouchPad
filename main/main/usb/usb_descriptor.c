#include <stdio.h>
#include <stdint.h>

#include "tinyusb.h"
#include "tusb.h"

#define REPORTID_TOUCHPAD         0x01
#define REPORTID_MOUSE            0x02  // 示例中通常是这样排列的
#define REPORTID_MAX_COUNT        0x03  // Device Capabilities
#define REPORTID_PTPHQA           0x04  // 认证相关 (一般返回全0即可)
#define REPORTID_FEATURE          0x05  // Input Mode
#define REPORTID_FUNCTION_SWITCH  0x06

#define EPNUM_TP_IN    0x81
#define EPNUM_MOUSE_IN 0x82

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + 2 * TUD_HID_DESC_LEN)

const uint8_t mouse_hid_report_descriptor[] = {

    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                         // USAGE (Mouse)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_MOUSE,               // REPORT_ID
    0x09, 0x01,                         // USAGE (Pointer)
    0xa1, 0x00,  

    0x05, 0x09,                         // USAGE_PAGE (Button)
    0x19, 0x01,                         // USAGE_MINIMUM (Button 1)
        0x29, 0x03,                     // USAGE_MAXIMUM (Button 3)
        0x15, 0x00,                     // LOGICAL_MINIMUM (0)
        0x25, 0x01,                     // LOGICAL_MAXIMUM (1)
        0x75, 0x01,                     // REPORT_SIZE (1)
        0x95, 0x03,                     // REPORT_COUNT (3)
        0x81, 0x02,                     // INPUT (Data,Var,Abs)
        0x95, 0x05,                     // REPORT_COUNT (5)
        0x81, 0x03,                     // INPUT (Cnst,Var,Abs)

        0x05, 0x01,                     // USAGE_PAGE (Generic Desktop)
        0x09, 0x30,                     // USAGE (X)
        0x09, 0x31,                     // USAGE (Y)
        0x15, 0x81,                     // LOGICAL_MINIMUM (-127)
        0x25, 0x7f,                     // LOGICAL_MAXIMUM (127)
        0x75, 0x08,                     // REPORT_SIZE (8)
        0x95, 0x02,                     // REPORT_COUNT (2)
        0x81, 0x06,                     // INPUT (Data,Var,Rel)
    0xC0,                               // END_COLLECTION (Physical)
    0xC0,                               // END_COLLECTION (Application)

};

const uint8_t ptp_hid_report_descriptor[] = {
//TOUCH PAD input TLC
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x05,                         // USAGE (Touch Pad)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_TOUCHPAD,            // REPORT_ID (Touch pad)

    // -------- Finger 0 --------
    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)

    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x47,                         // USAGE (Confidence)
    0x09, 0x42,                         // USAGE (Tip Switch)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x51,                         // USAGE (Contact Identifier)
    0x25, 0x3F,                         // LOGICAL_MAXIMUM (63)
    0x75, 0x06,                         // REPORT_SIZE (6)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- X Axis ----
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         // USAGE (X)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0x5F, 0x0E,                   // LOGICAL_MAXIMUM (3679)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- Y Axis ----
    0x09, 0x31,                         // USAGE (Y)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xD5, 0x08,                   // LOGICAL_MAXIMUM (2261)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0xC0,                               // END_COLLECTION

    // -------- Finger 1 --------
    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)

    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x47,                         // USAGE (Confidence)
    0x09, 0x42,                         // USAGE (Tip Switch)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x51,                         // USAGE (Contact Identifier)
    0x25, 0x3F,                         // LOGICAL_MAXIMUM (63)
    0x75, 0x06,                         // REPORT_SIZE (6)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- X Axis ----
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         // USAGE (X)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0x5F, 0x0E,                   // LOGICAL_MAXIMUM (3679)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- Y Axis ----
    0x09, 0x31,                         // USAGE (Y)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xD5, 0x08,                   // LOGICAL_MAXIMUM (2261)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0xC0,                               // END_COLLECTION

    // -------- Finger 2 --------
    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)

    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x47,                         // USAGE (Confidence)
    0x09, 0x42,                         // USAGE (Tip Switch)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x51,                         // USAGE (Contact Identifier)
    0x25, 0x3F,                         // LOGICAL_MAXIMUM (63)
    0x75, 0x06,                         // REPORT_SIZE (6)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- X Axis ----
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         // USAGE (X)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0x5F, 0x0E,                   // LOGICAL_MAXIMUM (3679)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- Y Axis ----
    0x09, 0x31,                         // USAGE (Y)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xD5, 0x08,                   // LOGICAL_MAXIMUM (2261)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0xC0,                               // END_COLLECTION

    // -------- Finger 3 --------
    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)

    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x47,                         // USAGE (Confidence)
    0x09, 0x42,                         // USAGE (Tip Switch)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x51,                         // USAGE (Contact Identifier)
    0x25, 0x3F,                         // LOGICAL_MAXIMUM (63)
    0x75, 0x06,                         // REPORT_SIZE (6)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- X Axis ----
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         // USAGE (X)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0x5F, 0x0E,                   // LOGICAL_MAXIMUM (3679)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- Y Axis ----
    0x09, 0x31,                         // USAGE (Y)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xD5, 0x08,                   // LOGICAL_MAXIMUM (2261)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0xC0,                               // END_COLLECTION

    // -------- Finger 4 --------
    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)

    0x05, 0x0D,                         // USAGE_PAGE (Digitizers)
    0x09, 0x47,                         // USAGE (Confidence)
    0x09, 0x42,                         // USAGE (Tip Switch)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0x09, 0x51,                         // USAGE (Contact Identifier)
    0x25, 0x3F,                         // LOGICAL_MAXIMUM (63)
    0x75, 0x06,                         // REPORT_SIZE (6)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- X Axis ----
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         // USAGE (X)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0x5F, 0x0E,                   // LOGICAL_MAXIMUM (3679)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    // ---- Y Axis ----
    0x09, 0x31,                         // USAGE (Y)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xD5, 0x08,                   // LOGICAL_MAXIMUM (2261)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)

    0xC0,                               // END_COLLECTION

    0x55, 0x0C,                         // UNIT_EXPONENT (-4)
    0x66, 0x01, 0x10,                   // UNIT (Seconds)
    0x47, 0xff, 0xff, 0x00, 0x00,       // PHYSICAL_MAXIMUM (65535)
    0x27, 0xff, 0xff, 0x00, 0x00,       // LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                         // REPORT_SIZE (16)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x56,                         // USAGE (Scan Time)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)
    0x09, 0x54,                         // USAGE (Contact count)
    0x25, 0x7f,                         // LOGICAL_MAXIMUM (127)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0x75, 0x08,                         // REPORT_SIZE (8)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)
    0x05, 0x09,                         // USAGE_PAGE (Button)
    0x09, 0x01,                         // USAGE_(Button 1)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x03,                         // REPORT_COUNT (3)
    0x81, 0x02,                         // INPUT (Data,Var,Abs)
    0x95, 0x05,                         // REPORT_COUNT (5)
    0x81, 0x03,                         // INPUT (Cnst,Var,Abs)
    0x05, 0x0d,                         // USAGE_PAGE (Digitizer)
    0x85, REPORTID_MAX_COUNT,           // REPORT_ID (Feature)
    0x09, 0x55,                         // USAGE (Contact Count Maximum)
    0x09, 0x59,                         // USAGE (Pad TYpe)
    0x75, 0x04,                         // REPORT_SIZE (4)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x25, 0x0f,                         // LOGICAL_MAXIMUM (15)
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs)
    0x06, 0x00, 0xff,                   // USAGE_PAGE (Vendor Defined)
    0x85, REPORTID_PTPHQA,              // REPORT_ID (PTPHQA)
    0x09, 0xC5,                         // USAGE (Vendor Usage 0xC5)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                   // LOGICAL_MAXIMUM (0xff)
    0x75, 0x08,                         // REPORT_SIZE (8)
    0x96, 0x00, 0x01,                   // REPORT_COUNT (0x100 (256))
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs)
    0xc0,                               // END_COLLECTION
    //CONFIG TLC
    0x05, 0x0d,                         // USAGE_PAGE (Digitizer)
    0x09, 0x0E,                         // USAGE (Configuration)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_FEATURE,             // REPORT_ID (Feature)
    0x09, 0x22,                         // USAGE (Finger)
    0xa1, 0x02,                         // COLLECTION (logical)
    0x09, 0x52,                         // USAGE (Input Mode)
    0x15, 0x00,                         // LOGICAL_MINIMUM (0)
    0x25, 0x0a,                         // LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         // REPORT_SIZE (8)
    0x95, 0x01,                         // REPORT_COUNT (1)
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs
    0xc0,                               // END_COLLECTION
    0x09, 0x22,                         // USAGE (Finger)
    0xa1, 0x00,                         // COLLECTION (physical)
    0x85, REPORTID_FUNCTION_SWITCH,     // REPORT_ID (Feature)
    0x09, 0x57,                         // USAGE(Surface switch)
    0x09, 0x58,                         // USAGE(Button switch)
    0x75, 0x01,                         // REPORT_SIZE (1)
    0x95, 0x02,                         // REPORT_COUNT (2)
    0x25, 0x01,                         // LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                         // FEATURE (Data,Var,Abs)
    0x95, 0x06,                         // REPORT_COUNT (6)
    0xb1, 0x03,                         // FEATURE (Cnst,Var,Abs)
    0xc0,                               // END_COLLECTION
    0xc0,                               // END_COLLECTION
};


// enum {
//     ITF_NUM_HID,
//     ITF_NUM_TOTAL
// };

// uint8_t const mouse_desc_configuration[] = {
//     TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
//     TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(mouse_hid_report_descriptor), 0x81, 64, 1)
// };

// uint8_t const ptp_desc_configuration[] = {
//     TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
//     TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(ptp_hid_report_descriptor), 0x81, 64, 1)
// };

uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, 2, 0, CONFIG_TOTAL_LEN, 0x00, 100),
    TUD_HID_DESCRIPTOR(0, 0, HID_ITF_PROTOCOL_NONE, sizeof(ptp_hid_report_descriptor), EPNUM_TP_IN, 64, 10),
    TUD_HID_DESCRIPTOR(1, 0, HID_ITF_PROTOCOL_MOUSE, sizeof(mouse_hid_report_descriptor), EPNUM_MOUSE_IN, 8, 10)
};