#include <Arduino.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_panel_io.h"
#ifndef ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP
#define ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP 0x14
#endif
#include "gt911_touch.h"

#include "pins_config.h"

static const char *TAG = "example";

esp_lcd_touch_handle_t tp;
esp_lcd_panel_io_handle_t tp_io_handle;

uint16_t touch_strength[1];
uint8_t touch_cnt = 0;

gt911_touch::gt911_touch(int8_t sda_pin, int8_t scl_pin, int8_t rst_pin, int8_t int_pin)
{
    _sda = sda_pin;
    _scl = scl_pin;
    _rst = rst_pin;
    _int = int_pin;
}

void gt911_touch::begin()
{
    // EV board BSP uses I2C1 for the shared codec/touch bus
    const i2c_port_t i2c_port = I2C_NUM_1;
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = (gpio_num_t)_sda,
        .scl_io_num = (gpio_num_t)_scl,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
    };
    i2c_conf.master.clk_speed = 400000; // 400kHz as in reference demo

    ESP_ERROR_CHECK(i2c_param_config(i2c_port, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_port, i2c_conf.mode, 0, 0, 0));

    // Probe both possible GT911 addresses before creating panel IO
    uint8_t dummy = 0;
    esp_err_t probe_5d = i2c_master_read_from_device(i2c_port, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, &dummy, 1, pdMS_TO_TICKS(20));
    esp_err_t probe_14 = i2c_master_read_from_device(i2c_port, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, &dummy, 1, pdMS_TO_TICKS(20));
    ESP_LOGI(TAG, "GT911 probe 0x5D -> 0x%x, 0x14 -> 0x%x", probe_5d, probe_14);

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    Serial0.printf("GT911 probe 0x5D -> 0x%x, 0x14 -> 0x%x\r\n", probe_5d, probe_14);
    esp_err_t err = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)i2c_port, &tp_io_config, &tp_io_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch IO init at addr 0x%02X failed (err=0x%x), retry backup addr 0x%02X", tp_io_config.dev_addr, err, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP);
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
        err = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)i2c_port, &tp_io_config, &tp_io_handle);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create touch IO, err=0x%x", err);
        Serial0.printf("Touch IO create failed, err=0x%x\r\n", err);
        tp_io_handle = NULL;
        return;
    }

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = (gpio_num_t)_rst,
        .int_gpio_num = (gpio_num_t)_int,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller gt911 (addr=0x%02X)", tp_io_config.dev_addr);
    Serial0.printf("GT911 init addr=0x%02X\r\n", tp_io_config.dev_addr);
    err = esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init GT911, err=0x%x", err);
        tp = NULL;
        return;
    }

    // Read product ID and config version for debugging
    uint8_t buf[4] = {0};
    if (esp_lcd_panel_io_rx_param(tp_io_handle, 0x8140, buf, 3) == ESP_OK) {
        Serial0.printf("GT911 ID: %02X %02X %02X\r\n", buf[0], buf[1], buf[2]);
    } else {
        Serial0.printf("GT911 ID read failed\r\n");
    }
    if (esp_lcd_panel_io_rx_param(tp_io_handle, 0x8047, buf, 1) == ESP_OK) {
        Serial0.printf("GT911 cfg ver: %u\r\n", buf[0]);
    } else {
        Serial0.printf("GT911 cfg read failed\r\n");
    }
}

bool gt911_touch::getTouch(uint16_t *x, uint16_t *y)
{
    static int err_logged = 0;
    static int dbg_left = 20;
    if (tp == NULL) {
        return false;
    }
    // Peek GT911 status register to see if controller reports data ready
    uint8_t status = 0;
    if (tp_io_handle && dbg_left > 0) {
        if (esp_lcd_panel_io_rx_param(tp_io_handle, 0x814E, &status, 1) == ESP_OK) {
            dbg_left--;
        }
    }
    esp_err_t err = esp_lcd_touch_read_data(tp);
    if (err != ESP_OK && err_logged < 10) {
        err_logged++;
    }
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(tp, x, y, touch_strength, &touch_cnt, 1);

    return touchpad_pressed;
}

void gt911_touch::set_rotation(uint8_t r){
switch(r){
    case 0:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, false);
        esp_lcd_touch_set_mirror_y(tp, false);
        break;
    case 1:
        esp_lcd_touch_set_swap_xy(tp, false);
        esp_lcd_touch_set_mirror_x(tp, true);
        esp_lcd_touch_set_mirror_y(tp, true);
        break;
    case 2:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, false);
        esp_lcd_touch_set_mirror_y(tp, false);
        break;
    case 3:
        esp_lcd_touch_set_swap_xy(tp, false);   
        esp_lcd_touch_set_mirror_x(tp, true);
        esp_lcd_touch_set_mirror_y(tp, true);
        break;
    }

}
