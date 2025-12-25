#ifndef USBHID_H
#define USBHID_H

void usbhid_task(void *arg);
void usbhid_init(void);

extern const uint8_t hid_report_descriptor[];
extern const uint8_t desc_configuration[];

#endif