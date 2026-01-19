#ifndef ELAN_I2C_H
#define ELAN_I2C_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

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

typedef struct {
    bool active;
    uint32_t down_time;
    uint16_t start_x;
    uint16_t start_y;
    uint16_t max_move;
    bool tap_detected;
} tp_finger_life_t;

extern QueueHandle_t tp_queue;
extern QueueHandle_t mouse_queue;
extern QueueSetHandle_t main_queue_set;

typedef enum {
    MOUSE_MODE = 0,
    PTP_MODE = 1
} input_mode_t;

extern volatile uint8_t current_mode;

void elan_i2c_task(void *arg);
void tp_interrupt_init(void);
void elan_i2c_init(void);

esp_err_t elan_activate_ptp();
esp_err_t elan_activate_mouse();

extern i2c_master_dev_handle_t dev_handle; 
extern i2c_master_bus_handle_t bus_handle;

#endif