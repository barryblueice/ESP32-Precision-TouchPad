idf_component_register(
    SRCS "main.c"
        "usb/usbhid.c"
        "usb/usb_descriptor.c"
        "nvs/ptp_nvs.c"
        "wireless/wifi_quene.c"
        "wireless/heartbeat.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio
        esp_driver_i2c
        esp_tinyusb
        esp_timer
        esp_wifi
        nvs_flash
    )
