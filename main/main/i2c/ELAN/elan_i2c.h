#ifndef ELAN_I2C_H
#define ELAN_I2C_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_timer.h"

void elan_i2c_task(void *arg);
void elan_tp_interrupt_init(void);
void elan_i2c_init(void);
void watchdog_timeout_callback(void* arg);

esp_err_t elan_activate_ptp();
esp_err_t elan_activate_mouse();

extern esp_timer_handle_t timeout_watchdog_timer;
extern bool global_watchdog_start;

#define WATCHDOG_TIMEOUT_US (100 * 1000)

#endif