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
    } fingers[5];
    uint8_t actual_count;
    uint8_t button_mask;
} tp_multi_msg_t;

extern QueueHandle_t tp_queue;

void elan_i2c_task(void *arg);
void tp_interrupt_init(void);

extern i2c_master_dev_handle_t dev_handle; 
extern i2c_master_bus_handle_t bus_handle;

#endif