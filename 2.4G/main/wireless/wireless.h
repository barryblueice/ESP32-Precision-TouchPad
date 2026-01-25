#ifndef WIRELESS_H
#define WIRELESS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t tp_queue;
extern QueueHandle_t mouse_queue;
extern QueueSetHandle_t main_queue_set;
extern uint32_t last_seen_timestamp;

typedef struct {
    struct {
        uint16_t x;
        uint16_t y;
        uint8_t  tip_switch;
        uint8_t  contact_id;
        uint8_t tip_switch_prev;
    } fingers[5];
    uint8_t actual_count;
    uint8_t button_mask;
} tp_multi_msg_t;

typedef struct __attribute__((packed)) {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
} mouse_msg_t;

typedef struct __attribute__((packed)) {
    uint8_t vbus_level;
} vbus_msg_t;

typedef struct __attribute__((packed)) {
    uint8_t vbus_level;
    uint8_t battery_level;
    uint32_t uptime;
} alive_msg_t;

typedef enum {
    MOUSE_MODE = 0,
    PTP_MODE = 1,
    VBUS_STATUS = 2,
    ALIVE_MODE = 3
} input_mode_t;

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

typedef struct __attribute__((packed)) {
    uint8_t buttons;
    int8_t  x;
    int8_t  y;
} mouse_hid_report_t;

typedef struct __attribute__((packed)) {
    input_mode_t type;
    union {
        mouse_hid_report_t mouse;
        ptp_report_t       ptp;
        vbus_msg_t         vbus;
        alive_msg_t        alive;
    } payload;
} wireless_msg_t;

extern volatile uint8_t current_mode;
void wifi_recieve_task_init();
void monitor_link_task(void *arg);

#endif