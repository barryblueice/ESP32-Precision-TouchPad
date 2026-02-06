#ifndef ELAN_I2C_H
#define ELAN_I2C_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void elan_i2c_task(void *arg);
void tp_interrupt_init(void);
void elan_i2c_init(void);

esp_err_t elan_activate_ptp();
esp_err_t elan_activate_mouse();

#endif