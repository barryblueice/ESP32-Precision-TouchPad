#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "class/hid/hid_device.h"

#include "math.h"

#define MY_REPORT_ID      0x01
#define TPD_REPORT_SIZE   6  // tip_switch(1) + x(2) + y(2) + contact_count(1)

static const char *TAG = "USB_HID_TP";

#define REPORTID_TOUCHPAD         0x01
#define REPORTID_MOUSE            0x02  // 示例中通常是这样排列的
#define REPORTID_MAX_COUNT        0x03  // Device Capabilities
#define REPORTID_PTPHQA           0x04  // 认证相关 (一般返回全0即可)
#define REPORTID_FEATURE          0x05  // Input Mode
#define REPORTID_FUNCTION_SWITCH  0x06  // Surface/Button Switch

// 最小 HID Touchpad Descriptor（Digitizer, Touchpad, Contact Count）
static const uint8_t hid_report_descriptor[] = {
//TOUCH PAD input TLC
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x05,                         // USAGE (Touch Pad)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_TOUCHPAD,            //   REPORT_ID (Touch pad)

    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x47,                         //   USAGE (Confidence)
    0x09, 0x42,                         //   USAGE (Tip Switch)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x02,                         //   REPORT_COUNT (2)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x51,                         //   USAGE (Contact Identifier)
    0x25, 0x3F,                         //   LOGICAL_MAXIMUM (63) -> 足够支持5指
    0x75, 0x06,                         //   REPORT_SIZE (6) -> 2+6=8bits，正好凑齐1字节
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x26, 0xFF, 0x0F,                   //   LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x09, 0x30,                         //   USAGE (X)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x31,                         //   USAGE (Y)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,                              //    END_COLLECTION

    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x47,                         //   USAGE (Confidence)
    0x09, 0x42,                         //   USAGE (Tip Switch)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x02,                         //   REPORT_COUNT (2)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x51,                         //   USAGE (Contact Identifier)
    0x25, 0x3F,                         //   LOGICAL_MAXIMUM (63) -> 足够支持5指
    0x75, 0x06,                         //   REPORT_SIZE (6) -> 2+6=8bits，正好凑齐1字节
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x26, 0xFF, 0x0F,                   //   LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x09, 0x30,                         //   USAGE (X)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x31,                         //   USAGE (Y)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,

    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x47,                         //   USAGE (Confidence)
    0x09, 0x42,                         //   USAGE (Tip Switch)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x02,                         //   REPORT_COUNT (2)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x51,                         //   USAGE (Contact Identifier)
    0x25, 0x3F,                         //   LOGICAL_MAXIMUM (63) -> 足够支持5指
    0x75, 0x06,                         //   REPORT_SIZE (6) -> 2+6=8bits，正好凑齐1字节
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x26, 0xFF, 0x0F,                   //   LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x09, 0x30,                         //   USAGE (X)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x31,                         //   USAGE (Y)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,

    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x47,                         //   USAGE (Confidence)
    0x09, 0x42,                         //   USAGE (Tip Switch)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x02,                         //   REPORT_COUNT (2)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x51,                         //   USAGE (Contact Identifier)
    0x25, 0x3F,                         //   LOGICAL_MAXIMUM (63) -> 足够支持5指
    0x75, 0x06,                         //   REPORT_SIZE (6) -> 2+6=8bits，正好凑齐1字节
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x26, 0xFF, 0x0F,                   //   LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x09, 0x30,                         //   USAGE (X)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x31,                         //   USAGE (Y)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,

    0x09, 0x22,                         // USAGE (Finger)
    0xA1, 0x02,                         // COLLECTION (Logical)
    0x05, 0x0D,                         //   USAGE_PAGE (Digitizers)
    0x09, 0x47,                         //   USAGE (Confidence)
    0x09, 0x42,                         //   USAGE (Tip Switch)
    0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //   REPORT_SIZE (1)
    0x95, 0x02,                         //   REPORT_COUNT (2)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x51,                         //   USAGE (Contact Identifier)
    0x25, 0x3F,                         //   LOGICAL_MAXIMUM (63) -> 足够支持5指
    0x75, 0x06,                         //   REPORT_SIZE (6) -> 2+6=8bits，正好凑齐1字节
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x05, 0x01,                         //   USAGE_PAGE (Generic Desktop)
    0x26, 0xFF, 0x0F,                   //   LOGICAL_MAXIMUM (4095)
    0x75, 0x10,                         //   REPORT_SIZE (16)
    0x95, 0x01,                         //   REPORT_COUNT (1)
    0x09, 0x30,                         //   USAGE (X)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0x09, 0x31,                         //   USAGE (Y)
    0x81, 0x02,                         //   INPUT (Data,Var,Abs)
    0xC0,

    0x55, 0x0C,                         //    UNIT_EXPONENT (-4)
    0x66, 0x01, 0x10,                   //    UNIT (Seconds)
    0x47, 0xff, 0xff, 0x00, 0x00,      //     PHYSICAL_MAXIMUM (65535)
    0x27, 0xff, 0xff, 0x00, 0x00,         //  LOGICAL_MAXIMUM (65535)
    0x75, 0x10,                           //  REPORT_SIZE (16)
    0x95, 0x01,                           //  REPORT_COUNT (1)
    0x05, 0x0d,                         //    USAGE_PAGE (Digitizers)
    0x09, 0x56,                         //    USAGE (Scan Time)
    0x81, 0x02,                           //  INPUT (Data,Var,Abs)
    0x09, 0x54,                         //    USAGE (Contact count)
    0x25, 0x7f,                           //  LOGICAL_MAXIMUM (127)
    0x95, 0x01,                         //    REPORT_COUNT (1)
    0x75, 0x08,                         //    REPORT_SIZE (8)
    0x81, 0x02,                         //    INPUT (Data,Var,Abs)
    0x05, 0x09,                         //    USAGE_PAGE (Button)
    0x09, 0x01,                         //    USAGE_(Button 1)
    0x09, 0x02,                         //    USAGE_(Button 2)
    0x09, 0x03,                         //    USAGE_(Button 3)
    0x25, 0x01,                         //    LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //    REPORT_SIZE (1)
    0x95, 0x03,                         //    REPORT_COUNT (3)
    0x81, 0x02,                         //    INPUT (Data,Var,Abs)
    0x95, 0x05,                          //   REPORT_COUNT (5)
    0x81, 0x03,                         //    INPUT (Cnst,Var,Abs)
    0x05, 0x0d,                         //    USAGE_PAGE (Digitizer)
    0x85, REPORTID_MAX_COUNT,            //   REPORT_ID (Feature)
    0x09, 0x55,                         //    USAGE (Contact Count Maximum)
    0x09, 0x59,                         //    USAGE (Pad TYpe)
    0x75, 0x04,                         //    REPORT_SIZE (4)
    0x95, 0x02,                         //    REPORT_COUNT (2)
    0x25, 0x0f,                         //    LOGICAL_MAXIMUM (15)
    0xb1, 0x02,                         //    FEATURE (Data,Var,Abs)
    0x06, 0x00, 0xff,                   //    USAGE_PAGE (Vendor Defined)
    0x85, REPORTID_PTPHQA,               //    REPORT_ID (PTPHQA)
    0x09, 0xC5,                         //    USAGE (Vendor Usage 0xC5)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                   //    LOGICAL_MAXIMUM (0xff)
    0x75, 0x08,                         //    REPORT_SIZE (8)
    0x96, 0x00, 0x01,                   //    REPORT_COUNT (0x100 (256))
    0xb1, 0x02,                         //    FEATURE (Data,Var,Abs)
    0xc0,                               // END_COLLECTION
    //CONFIG TLC
    0x05, 0x0d,                         //    USAGE_PAGE (Digitizer)
    0x09, 0x0E,                         //    USAGE (Configuration)
    0xa1, 0x01,                         //   COLLECTION (Application)
    0x85, REPORTID_FEATURE,             //   REPORT_ID (Feature)
    0x09, 0x22,                         //   USAGE (Finger)
    0xa1, 0x02,                         //   COLLECTION (logical)
    0x09, 0x52,                         //    USAGE (Input Mode)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)
    0x25, 0x0a,                         //    LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         //    REPORT_SIZE (8)
    0x95, 0x01,                         //    REPORT_COUNT (1)
    0xb1, 0x02,                         //    FEATURE (Data,Var,Abs
    0xc0,                               //   END_COLLECTION
    0x09, 0x22,                         //   USAGE (Finger)
    0xa1, 0x00,                         //   COLLECTION (physical)
    0x85, REPORTID_FUNCTION_SWITCH,     //     REPORT_ID (Feature)
    0x09, 0x57,                         //     USAGE(Surface switch)
    0x09, 0x58,                         //     USAGE(Button switch)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0xb1, 0x02,                         //     FEATURE (Data,Var,Abs)
    0x95, 0x06,                         //     REPORT_COUNT (6)
    0xb1, 0x03,                         //     FEATURE (Cnst,Var,Abs)
    0xc0,                               //   END_COLLECTION
    0xc0,                               // END_COLLECTION
    //MOUSE TLC
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
    0x09, 0x02,                         // USAGE (Mouse)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_MOUSE,               //   REPORT_ID (Mouse)
    0x09, 0x01,                         //   USAGE (Pointer)
    0xa1, 0x00,                         //   COLLECTION (Physical)
    0x05, 0x09,                         //     USAGE_PAGE (Button)
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                         //     REPORT_SIZE (1)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)
    0x95, 0x06,                         //     REPORT_COUNT (6)
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                         //     USAGE (X)
    0x09, 0x31,                         //     USAGE (Y)
    0x75, 0x10,                         //     REPORT_SIZE (16)
    0x95, 0x02,                         //     REPORT_COUNT (2)
    0x25, 0x0a,                          //    LOGICAL_MAXIMUM (10)
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)
    0xc0,                               //   END_COLLECTION
    0xc0,                                //END_COLLECTION
};

#define TPD_REPORT_ID 0x01
// 注意：tud_hid_report 发送时不包含 ID 字节，所以给 TinyUSB 的长度是不含 ID 的
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

// Configuration Descriptor
uint8_t const desc_configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    // 修改最后一个参数为 REPORT_SIZE
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE,
                       sizeof(hid_report_descriptor), 0x81, 16, 1)
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
        // ID 5: Input Mode
        if (report_id == REPORTID_FEATURE) {
            buffer[0] = 0x03; // 0x03 = PTP Mode
            return 1;
        }
        // ID 3: Capabilities (Contact Count Max & Pad Type)
        if (report_id == REPORTID_MAX_COUNT) {
            // 描述符定义了两个 4-bit 字段
            // Low nibble: Contact Count Max (支持5指)
            // High nibble: Pad Type (0 = Touchpad)
            buffer[0] = 0x05; 
            return 1;
        }
        // ID 4: PTPHQA (Certification)
        if (report_id == REPORTID_PTPHQA) {
            // 直接返回全0即可，通常 Windows 只是读取一下
            memset(buffer, 0, reqlen > 256 ? 256 : reqlen);
            return reqlen > 256 ? 256 : reqlen;
        }
    }
    return 0;
}

static uint8_t ptp_input_mode = 0x00;

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) {
    (void)instance;
    
    // 检查是否是针对模式切换的 Feature Report
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == REPORTID_FEATURE) {
        if (bufsize >= 1) {
            ptp_input_mode = buffer[0];
        }
    }
}

// 初始化 USB HID
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

// typedef struct __attribute__((packed)) {
//     // --- Byte 0: Finger Collection ---
//     uint8_t confidence   : 1; // Bit 0: Confidence
//     uint8_t tip_switch   : 1; // Bit 1: Tip Switch
//     uint8_t contact_id   : 2; // Bit 2-3: Contact ID (Logical Max 2)
//     uint8_t padding      : 4; // Bit 4-7: Padding (Const)

//     // --- Byte 1-2: X ---
//     uint16_t x;               // Logical Max 4095

//     // --- Byte 3-4: Y ---
//     uint16_t y;               // Logical Max 4095

//     // --- Byte 5-6: Scan Time ---
//     // 你描述符中定义在 Finger Collection 结束后，紧跟 16bit 的 Scan Time
//     uint16_t scan_time;

//     // --- Byte 7: Contact Count ---
//     // 你描述符中定义为 8bit
//     uint8_t contact_count;

//     // --- Byte 8: Buttons ---
//     uint8_t button_1     : 1; // Bit 0
//     uint8_t button_2     : 1; // Bit 1
//     uint8_t button_3     : 1; // Bit 2
//     uint8_t btn_padding  : 5; // Bit 3-7: Padding 5 bits

// } ptp_report_t;

// void simulate_circle_move() {
//     static float angle = 0.0f;
//     static uint16_t timestamp = 0;
    
//     // --- 调速区 ---
//     const float SPEED = 0.04f;   // 因为频率翻倍了，所以 SPEED 要减半才维持原速
//     const float SIZE = 400.0f;    
//     // --------------

//     ptp_report_t report = {0};
//     report.confidence = 1;
//     report.tip_switch = 1;
//     report.contact_id = 0; 
    
//     report.x = (uint16_t)(2048.0f + cosf(angle) * SIZE);
//     report.y = (uint16_t)(2048.0f + sinf(angle) * SIZE);
    
//     // 重点：5ms 周期对应增加 50，让 Windows 认为数据是连续且等速的
//     report.scan_time = timestamp;
//     timestamp += 50; 
    
//     report.contact_count = 1; 

//     tud_hid_report(0x01, &report, sizeof(report));

//     angle += SPEED;
//     if (angle > 6.283185f) angle -= 6.283185f;
// }

typedef struct __attribute__((packed)) {
    // 状态字节：Bit0:Confidence, Bit1:TipSwitch, Bit2-7:ContactID
    uint8_t tip_conf_id; 
    uint16_t x;
    uint16_t y;
} finger_t;

typedef struct __attribute__((packed)) {
    finger_t fingers[5];    // 描述符复制5遍，这里就对应数组5个元素
    uint16_t scan_time;     
    uint8_t contact_count;  
    uint8_t buttons;        
} ptp_report_t;

void simulate_dual_scroll() {
    static uint16_t scroll_y = 1000;
    static uint16_t timestamp = 0;
    ptp_report_t report = {0};

    // 第一根手指 (ID = 0)
    report.fingers[0].tip_conf_id = (0 << 2) | 0x03; // ID=0, Confidence=1, Tip=1
    report.fingers[0].x = 1000;
    report.fingers[0].y = scroll_y;

    // 第二根手指 (ID = 1)
    report.fingers[1].tip_conf_id = (1 << 2) | 0x03; // ID=1, Confidence=1, Tip=1
    report.fingers[1].x = 2000; // 横向拉开距离
    report.fingers[1].y = scroll_y;

    report.contact_count = 2; // 告诉 Windows 有两根手指
    report.scan_time = timestamp;
    timestamp += 50;

    // 发送报表 (ID = 0x01)
    tud_hid_report(0x01, &report, sizeof(report));

    // 改变 Y 轴坐标实现滚动
    scroll_y -= 10; 
    if (scroll_y > 3000) scroll_y = 1000;
}

// 主循环任务
void usbhid_task(void *arg) {
    ESP_LOGI(TAG, "USB HID Task Started");
    
    // 1. 必须先获取当前时间初始化
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    // 2. 确保频率不为 0。如果你的 Tick 是 100Hz，pdMS_TO_TICKS(5) 会变成 0
    // 建议至少使用 10ms，或者在 menuconfig 中调高 FreeRTOS HZ
    const TickType_t xFrequency = pdMS_TO_TICKS(5); 

    while (1) {
        // 使用 vTaskDelayUntil 保证频率稳定
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        if (tud_hid_ready()) {
            // simulate_circle_move();
            // simulate_dual_scroll();
        }
    }
}