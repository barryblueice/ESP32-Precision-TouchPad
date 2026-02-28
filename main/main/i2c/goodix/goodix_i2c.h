#ifndef GOODIX_I2C_H
#define GOODIX_I2C_H

#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void goodix_i2c_task(void *arg);
void goodix_tp_interrupt_init(void);
void goodix_i2c_init(void);
void watchdog_timeout_callback(void* arg);

esp_err_t goodix_activate_ptp();
esp_err_t goodix_activate_mouse();

extern esp_timer_handle_t timeout_watchdog_timer;
extern bool global_watchdog_start;

#define WATCHDOG_TIMEOUT_US (100 * 1000)

#endif