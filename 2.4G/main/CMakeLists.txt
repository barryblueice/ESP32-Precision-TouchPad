idf_component_register(
    SRCS "main.c"
        "usb/usbhid.c"
        "usb/usb_descriptor.c"
        "i2c/elan_i2c.c"
        "i2c/i2c_int.c"
        "nvs/ptp_nvs.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio
        esp_driver_i2c
        esp_tinyusb
        esp_timer
        esp_wifi
        nvs_flash
    )
