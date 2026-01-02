#ifndef USBHID_H
#define USBHID_H

void usbhid_task(void *arg);
void usbhid_init(void);
void usb_mount_task(void *arg);

extern const uint8_t ptp_hid_report_descriptor[];
extern const uint8_t mouse_hid_report_descriptor[];
extern const uint8_t desc_configuration[];

#endif