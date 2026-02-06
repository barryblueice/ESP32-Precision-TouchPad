#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ELAN_PTP_FINAL";

#define I2C_ADDR    0x15
#define SCL_IO      9
#define SDA_IO      8
#define RST_IO      6
#define INT_IO      7

/* ---------- I2C ---------- */
i2c_master_dev_handle_t dev_handle = NULL;
i2c_master_bus_handle_t bus_handle = NULL;

uint8_t *report_desc = NULL;
int report_desc_len = 0;

typedef struct {
    uint16_t max_x;
    uint16_t max_y;

    uint8_t raw_x_lo;
    uint8_t raw_x_hi;

    uint8_t raw_y_lo;
    uint8_t raw_y_hi;
} tp_max_xy_t;

static tp_max_xy_t g_max_xy = {0, 0};

typedef struct {
    uint8_t tip_offset;
    uint8_t id_offset;
    uint8_t x_offset;
    uint8_t y_offset;
    uint8_t block_size;
    uint16_t x_size;
    uint16_t y_size;
    bool nibble_packed;
} finger_layout_t;

typedef struct {
    struct {
        uint16_t x;
        uint16_t y;
        uint8_t  tip_switch;
        uint8_t  contact_id;
    } fingers[5];
    uint8_t actual_count;
    uint8_t button_mask;
} tp_multi_msg_t;


static void hex_dump(const uint8_t *buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02X ", buf[i]);
        if ((i & 0x0F) == 0x0F) printf("\n");
    }
    printf("\n");
}

static tp_max_xy_t elan_parse_max_xy(const uint8_t *desc, int len)
{
    tp_max_xy_t max_val = {0, 0, 0, 0};

    bool in_x = false;
    bool in_y = false;

    for (int i = 0; i < len - 3; i++) {

        /* 进入 X / Y Usage */
        if (desc[i] == 0x09 && desc[i + 1] == 0x30) { // USAGE X
            in_x = true;
            in_y = false;
            continue;
        }
        if (desc[i] == 0x09 && desc[i + 1] == 0x31) { // USAGE Y
            in_y = true;
            in_x = false;
            continue;
        }

        /* 只接受 16bit LOGICAL_MAXIMUM */
        if (desc[i] == 0x26) {

            if (in_x) {
                /* 原样保存 descriptor 字节 */
                max_val.raw_x_lo = desc[i + 1];
                max_val.raw_x_hi = desc[i + 2];

                /* 现有逻辑仍然可以用解码值 */
                uint16_t value = desc[i + 1] | (desc[i + 2] << 8);
                if (value > max_val.max_x) {
                    max_val.max_x = value;
                }
            }

            if (in_y) {
                max_val.raw_y_lo = desc[i + 1];
                max_val.raw_y_hi = desc[i + 2];

                uint16_t value = desc[i + 1] | (desc[i + 2] << 8);
                if (value > max_val.max_y) {
                    max_val.max_y = value;
                }
            }
        }

    }

    ESP_LOGI(TAG,
            "X: max=%d raw=[0x%02X 0x%02X], Y: max=%d raw=[0x%02X 0x%02X]",
            max_val.max_x,
            max_val.raw_x_lo, max_val.raw_x_hi,
            max_val.max_y,
            max_val.raw_y_lo, max_val.raw_y_hi);

    return max_val;
}


esp_err_t elan_activate_ptp_payload() {
    uint8_t payload[] = {
        0x05, 0x00, 0x33, 0x03, 0x06,
        0x00, 0x05, 0x00, 0x03, 0x03, 0x00
    };
    ESP_LOGI(TAG, "Sending PTP Activation Payload");
    return i2c_master_transmit(dev_handle, payload, sizeof(payload), 200);
}

static bool elan_read_descriptors() {
    esp_err_t ret;
    uint8_t hid_desc[32] = {0};
    uint8_t req[2] = {0x01, 0x00};

    ret = i2c_master_transmit_receive(
        dev_handle, req, 2, hid_desc, 30, pdMS_TO_TICKS(200)
    );
    if (ret != ESP_OK) return false;

    report_desc_len = hid_desc[4] | (hid_desc[5] << 8);
    uint16_t report_reg = hid_desc[6] | (hid_desc[7] << 8);

    if (report_desc_len == 0 || report_desc_len > 1024) return false;

    report_desc = malloc(report_desc_len);
    if (!report_desc) return false;

    uint8_t reg[2] = { report_reg & 0xFF, report_reg >> 8 };
    ret = i2c_master_transmit_receive(
        dev_handle, reg, 2, report_desc, report_desc_len, pdMS_TO_TICKS(500)
    );
    if (ret != ESP_OK) {
        free(report_desc);
        report_desc = NULL;
        return false;
    }

    ESP_LOGI(TAG, "Report Descriptor (%d bytes)", report_desc_len);
    hex_dump(report_desc, report_desc_len);
    return true;
}

static finger_layout_t parse_finger_layout(void) {
    finger_layout_t layout = {0};

    layout.tip_offset = 0;
    layout.id_offset  = 1;
    layout.x_offset   = 2;
    layout.y_offset   = 4;
    layout.block_size = 6;
    layout.nibble_packed = false;

    layout.x_size = g_max_xy.max_x;
    layout.y_size = g_max_xy.max_y;

    ESP_LOGI(TAG, "Finger Layout: X=%d Y=%d",
             layout.x_size, layout.y_size);

    return layout;
}

static void parse_ptp_report(uint8_t *data, int len,
                             finger_layout_t layout,
                             tp_multi_msg_t *msg_out) {

    if (data[2] != 0x04) return;

    static struct {
        uint16_t x;
        uint16_t y;
        bool active;
    } slots[5] = {0};

    uint8_t status = data[3];
    uint8_t cid = (status >> 4) & 0x0F;
    bool tip = status & 0x01;

    uint16_t x = data[4] | (data[5] << 8);
    uint16_t y = data[6] | (data[7] << 8);

    if (x > layout.x_size) x = layout.x_size;
    if (y > layout.y_size) y = layout.y_size;

    if (cid < 5) {
        slots[cid].x = x;
        slots[cid].y = y;
        slots[cid].active = tip;
    }

    memset(msg_out, 0, sizeof(*msg_out));

    for (int i = 0; i < 5; i++) {
        if (slots[i].active) {
            int n = msg_out->actual_count++;
            msg_out->fingers[n].x = slots[i].x;
            msg_out->fingers[n].y = slots[i].y;
            msg_out->fingers[n].contact_id = i;
            msg_out->fingers[n].tip_switch = 1;
        }
    }

    if (msg_out->actual_count) {
        printf("[Touch] count=%d ", msg_out->actual_count);
        for (int i = 0; i < msg_out->actual_count; i++) {
            printf("F%d(%d,%d) ",
                msg_out->fingers[i].contact_id,
                msg_out->fingers[i].x,
                msg_out->fingers[i].y);
        }
        printf("\n");
    }
}

void elan_init_sequence(void) {
    gpio_set_direction(RST_IO, GPIO_MODE_OUTPUT);
    gpio_set_level(RST_IO, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_IO, 1);
    vTaskDelay(pdMS_TO_TICKS(150));

    elan_activate_ptp_payload();
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!elan_read_descriptors()) {
        ESP_LOGE(TAG, "Descriptor read failed");
        return;
    }

    g_max_xy = elan_parse_max_xy(report_desc, report_desc_len);
}

void elan_i2c_task(void *arg) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_IO,
        .sda_io_num = SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_ADDR,
        .scl_speed_hz = 400000,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    gpio_set_direction(INT_IO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(INT_IO, GPIO_PULLUP_ONLY);

    elan_init_sequence();
    finger_layout_t layout = parse_finger_layout();

    uint8_t data[64];
    tp_multi_msg_t msg;

    while (1) {
        if (gpio_get_level(INT_IO) == 0) {
            if (i2c_master_receive(dev_handle, data, sizeof(data),
                                   pdMS_TO_TICKS(10)) == ESP_OK) {
                parse_ptp_report(data, sizeof(data), layout, &msg);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void app_main(void) {
    xTaskCreate(elan_i2c_task, "elan_i2c_task",
                8192, NULL, 10, NULL);
}
