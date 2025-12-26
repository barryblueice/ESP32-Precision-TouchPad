#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

#include "i2c/elan_i2c.h"

#define TAG "TP_INT"

extern i2c_master_dev_handle_t dev_handle;
extern TaskHandle_t tp_read_task_handle;

static void IRAM_ATTR tp_gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    if (tp_read_task_handle != NULL) {
        vTaskNotifyGiveFromISR(tp_read_task_handle, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

void tp_interrupt_init(void) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_NUM_7),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0); 
    gpio_isr_handler_add(GPIO_NUM_7, tp_gpio_isr_handler, NULL);
}