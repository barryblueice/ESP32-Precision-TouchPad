#ifndef PTP_NVS_H
#define PTP_NVS_H

#include "esp_log.h"

esp_err_t nvs_mode_init(void);
esp_err_t nvs_mode_write(uint8_t mode);
esp_err_t nvs_mode_read(uint8_t* mode);

#endif