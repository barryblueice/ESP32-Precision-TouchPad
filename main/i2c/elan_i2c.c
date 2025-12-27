#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "i2c/elan_i2c.h"

#include "usb/usbhid.h"

static const char *TAG = "ELAN_PTP";

QueueHandle_t tp_queue = NULL;

#define I2C_ADDR 0x15
#define RST_IO   6
#define INT_IO   7

i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;

TaskHandle_t tp_read_task_handle = NULL;
#define TAP_MOVE_THRESHOLD 10
#define TAP_TIME_THRESHOLD 150
#define DOUBLE_TAP_WINDOW  50

static esp_err_t elan_activate_ptp() {
    uint8_t payload[] = {
        0x05, 0x00,             // Command Register
        0x33, 0x03,             // SET_REPORT Feature ID 03
        0x06, 0x00,
        0x05, 0x00,             // Usage Page: Digitizer
        0x03, 0x03, 0x00        // Value: Enable PTP Mode
    };
    ESP_LOGI(TAG, "Activating PTP Mode...");
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

typedef struct {
    uint8_t tip_offset;
    uint8_t id_offset;
    uint8_t x_offset;
    uint8_t y_offset;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t block_size;
    bool nibble_packed;
} finger_layout_t;

static void elan_i2c_init(void) {
    gpio_set_direction(RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 9,
        .sda_io_num = 8,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 800000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    uint8_t pwr_on[] = {0x05, 0x00, 0x08, 0x00};
    i2c_master_transmit(dev_handle, pwr_on, 4, 100);
    vTaskDelay(pdMS_TO_TICKS(20));

    elan_activate_ptp();
    vTaskDelay(pdMS_TO_TICKS(100));

    gpio_set_direction(INT_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INT_IO, GPIO_PULLUP_ONLY);
}

// PTP 数据解析 (Tip/ID/XY)
static void parse_ptp_report(uint8_t *data, int len, finger_layout_t layout, tp_multi_msg_t *msg_out)  {
    uint8_t report_id = data[2];
    if (report_id != 0x04) return;

    uint8_t status = data[3];
    uint8_t contact_id = (status & 0xF0) >> 4;
    bool tip = (status & 0x01);
    uint16_t raw_x = data[4] | (data[5] << 8);
    uint16_t raw_y = data[6] | (data[7] << 8);
    
    bool physical_pressed = (data[11] == 0x81);
    static uint8_t locked_button = 0;
    static bool is_detecting_click = false;

    if (physical_pressed) {
        if (!is_detecting_click) {
            is_detecting_click = true;
            locked_button = (raw_x > 1700) ? 0x02 : 0x01;
        }
        msg_out->button_mask = locked_button;
    } else {
        is_detecting_click = false;
        locked_button = 0;
        msg_out->button_mask = 0;
    }

    if (contact_id < 5) {
        msg_out->fingers[contact_id].x = raw_x;
        msg_out->fingers[contact_id].y = raw_y;
        msg_out->fingers[contact_id].tip_switch = tip ? 1 : 0;
        msg_out->fingers[contact_id].contact_id = contact_id;
    }

    uint8_t count = 0;
    for (int i = 0; i < 5; i++) {
        if (msg_out->fingers[i].tip_switch) {
            count++;
        }
    }
    msg_out->actual_count = count;
}

void elan_i2c_task(void *arg) {
    elan_i2c_init();
    tp_interrupt_init();

    tp_multi_msg_t current_state; 
    uint8_t data[64];
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint8_t last_count = 0;
    static uint8_t last_buttons = 0;

    while (1) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10)); 

        for (int i = 0; i < 5; i++) {
            current_state.fingers[i].tip_switch = 0;
        }
        current_state.actual_count = 0;
        current_state.button_mask = 0;
        bool has_data = false;

        while (gpio_get_level(INT_IO) == 0) {
            if (i2c_master_receive(dev_handle, data, sizeof(data), pdMS_TO_TICKS(2)) == ESP_OK) {
                
                current_state.button_mask = (data[11] == 0x81) ? 0x01 : 0x00;

                if (data[2] == 0x04) {
                    has_data = true;
                    uint8_t status = data[3];
                    uint8_t id = (status & 0xF0) >> 4;
                    
                    if (id < 5) {
                        current_state.fingers[id].tip_switch = (status & 0x01);
                        current_state.fingers[id].x = data[4] | (data[5] << 8);
                        current_state.fingers[id].y = data[6] | (data[7] << 8);
                        current_state.fingers[id].contact_id = id;
                    }
                }
            } else {
                break;
            }
        }

        uint8_t count = 0;
        for (int i = 0; i < 5; i++) {
            if (current_state.fingers[i].tip_switch) count++;
        }
        current_state.actual_count = count;

        if (has_data || 
            current_state.button_mask != last_buttons || 
            (current_state.actual_count == 0 && last_count > 0)) 
        {
            xQueueOverwrite(tp_queue, &current_state);
        }

        last_count = current_state.actual_count;
        last_buttons = current_state.button_mask;

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); 
    }
}
