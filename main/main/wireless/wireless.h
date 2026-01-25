#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>

extern uint8_t wireless_mode;
extern uint8_t receiver_mac[6];

void vbus_det_init(void);
void alive_heartbeat_task(void *pvParameters);
void wireless_init();

#endif