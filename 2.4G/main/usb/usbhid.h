#ifndef USBHID_H
#define USBHID_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void usbhid_task(void *arg);
void usbhid_init(void);
void usb_mount_task(void *arg);

extern const uint8_t ptp_hid_report_descriptor[];
extern const uint8_t mouse_hid_report_descriptor[];
extern const uint8_t generic_hid_report_descriptor[];
extern const uint8_t desc_configuration[];
extern EventGroupHandle_t usb_event_group;

#endif