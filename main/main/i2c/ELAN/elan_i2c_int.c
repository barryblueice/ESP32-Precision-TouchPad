#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "i2c/ELAN/elan_i2c.h"
#include "i2c/I2C_HID_Report.h"

#define TAG "TP_INT"

extern i2c_master_dev_handle_t dev_handle;
extern TaskHandle_t tp_read_task_handle;

static void IRAM_ATTR tp_gpio_isr_handler(void* arg) {
    uint8_t level = gpio_get_level(GPIO_NUM_7);
    if (level == 0) {
        esp_timer_stop(timeout_watchdog_timer);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (tp_read_task_handle != NULL) {
            vTaskNotifyGiveFromISR(tp_read_task_handle, &xHigherPriorityTaskWoken);
        }
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    } else {
        esp_timer_start_once(timeout_watchdog_timer, WATCHDOG_TIMEOUT_US);
    }
}

void elan_tp_interrupt_init(void) {

    const esp_timer_create_args_t timer_args = {
        .callback = &watchdog_timeout_callback,
        .name = "touch_timeout_timer"
    };
    esp_timer_create(&timer_args, &timeout_watchdog_timer);

    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_7),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0); 
    gpio_isr_handler_add(GPIO_NUM_7, tp_gpio_isr_handler, NULL);
}