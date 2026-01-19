#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char* TAG = "NVS_MODE";

#define NVS_NAMESPACE "usb_mode_ns"
#define NVS_KEY       "current_mode"

esp_err_t nvs_mode_write(uint8_t mode) {
    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed (%d)", ret);
        return ret;
    }

    ret = nvs_set_u8(handle, NVS_KEY, mode);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed (%d)", ret);
        nvs_close(handle);
        return ret;
    }

    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed (%d)", ret);
    }

    nvs_close(handle);
    return ret;
}

esp_err_t nvs_mode_read(uint8_t* mode) {
    if (!mode) return ESP_ERR_INVALID_ARG;

    nvs_handle_t handle;
    esp_err_t ret;

    ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed (%d)", ret);
        return ret;
    }

    ret = nvs_get_u8(handle, NVS_KEY, mode);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGW(TAG, "NVS key not found, returning default");
        } else {
            ESP_LOGE(TAG, "NVS get failed (%d)", ret);
        }
    }

    nvs_close(handle);
    return ret;
}

esp_err_t nvs_mode_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    uint8_t mode = 0;

    ret = nvs_mode_read(&mode);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "NVS key not found, pre-storing default mode 0x00");
        nvs_mode_write(0x00);
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed (%d)", ret);
    }
    return ret;
}
