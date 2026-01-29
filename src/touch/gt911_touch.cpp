#ifndef USE_LGFX_TOUCH
#define USE_LGFX_TOUCH 0
#endif

#if !USE_LGFX_TOUCH
#include <Arduino.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
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
static i2c_master_bus_handle_t i2c_bus = nullptr;

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
    if (!i2c_bus) {
        i2c_master_bus_config_t i2c_conf = {};
        i2c_conf.i2c_port = I2C_NUM_1;
        i2c_conf.sda_io_num = (gpio_num_t)_sda;
        i2c_conf.scl_io_num = (gpio_num_t)_scl;
        i2c_conf.clk_source = I2C_CLK_SRC_DEFAULT;
        i2c_conf.glitch_ignore_cnt = 7;
        i2c_conf.intr_priority = 0;
        i2c_conf.trans_queue_depth = 4;
        i2c_conf.flags.enable_internal_pullup = 1;
        ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_conf, &i2c_bus));
    }

    // Probe both possible GT911 addresses before creating panel IO
    esp_err_t probe_5d = i2c_master_probe(i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS, 20);
    esp_err_t probe_14 = i2c_master_probe(i2c_bus, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP, 20);
    ESP_LOGI(TAG, "GT911 probe 0x5D -> 0x%x, 0x14 -> 0x%x", probe_5d, probe_14);

    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();
    if (probe_5d == ESP_OK) {
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS;
    } else if (probe_14 == ESP_OK) {
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
    } else {
        ESP_LOGE(TAG, "GT911 not found on I2C (0x5D/0x14). Check wiring/pullups.");
        Serial0.println("GT911 not found on I2C (0x5D/0x14).");
        return;
    }
    tp_io_config.lcd_param_bits = 8;    // GT911 uses 8-bit data
    tp_io_config.scl_speed_hz = 100000; // start conservative for stability
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    Serial0.printf("GT911 probe 0x5D -> 0x%x, 0x14 -> 0x%x\r\n", probe_5d, probe_14);
    esp_err_t err = esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Touch IO init at addr 0x%02X failed (err=0x%x), retry backup addr 0x%02X", tp_io_config.dev_addr, err, ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP);
        tp_io_config.dev_addr = ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP;
        err = esp_lcd_new_panel_io_i2c(i2c_bus, &tp_io_config, &tp_io_handle);
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
#endif
