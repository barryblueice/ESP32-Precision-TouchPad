set(srcs
    "main.c"
    "usb/usbhid.c"
    "usb/usb_descriptor.c"
    "wireless/vbus_det.c"
    "wireless/heartbeat.c"
    "wireless/broadcast.c"
    "nvs/ptp_nvs.c"
)

if(CONFIG_ELAN_LENOVO_33370A)
    list(APPEND srcs 
        "i2c/ELAN/elan_i2c.c"
        "i2c/ELAN/elan_i2c_int.c"
        "i2c/ELAN/elan_watchdog.c"
    )
endif()

if(CONFIG_MI_GOODIX_HAPTIC_ENGINE)
    list(APPEND srcs 
        "i2c/goodix/goodix_i2c.c"
        "i2c/goodix/goodix_i2c_int.c"
        "i2c/goodix/goodix_watchdog.c"
    )
endif()

idf_component_register(
    SRCS "${srcs}"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        esp_driver_gpio
        esp_driver_i2c
        esp_tinyusb
        esp_timer
        nvs_flash
        esp_event
        esp_wifi
)