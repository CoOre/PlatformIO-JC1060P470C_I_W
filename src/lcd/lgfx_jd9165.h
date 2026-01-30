#pragma once

#define LGFX_USE_V1
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>
#include <lgfx/v1/touch/Touch_GT911.hpp>
#include <esp_err.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_mipi_dsi.h>
#include "pins_config.h"

namespace lgfx
{
inline namespace v1
{

struct Panel_JD9165_Exact : public Panel_DSI
{
public:
    Panel_JD9165_Exact()
    {
        auto cfg = config();
        cfg.panel_width = LCD_H_RES;
        cfg.panel_height = LCD_V_RES;
        cfg.pin_rst = LCD_RST;
        cfg.readable = false;
        cfg.invert = false;
        cfg.rgb_order = 0;
        cfg.offset_rotation = 0;
        config(cfg);

        auto detail = config_detail();
        detail.dpi_freq_mhz = 50;
        detail.hsync_pulse_width = 40;
        detail.hsync_back_porch = 160;
        detail.hsync_front_porch = 160;
        detail.vsync_pulse_width = 10;
        detail.vsync_back_porch = 23;
        detail.vsync_front_porch = 12;
        config_detail(detail);

        _backlight_pin = LCD_LED;
    }

    const uint8_t *getInitParams(size_t listno) const override
    {
        static constexpr uint8_t list0[] = {
            2, 0x30, 0x00,
            5, 0xF7, 0x49, 0x61, 0x02, 0x00,
            2, 0x30, 0x01,
            2, 0x04, 0x0C,
            2, 0x05, 0x00,
            2, 0x06, 0x00,
            2, 0x0B, 0x11,
            2, 0x17, 0x00,
            2, 0x20, 0x04,
            2, 0x1F, 0x05,
            2, 0x23, 0x00,
            2, 0x25, 0x19,
            2, 0x28, 0x18,
            2, 0x29, 0x04,
            2, 0x2A, 0x01,
            2, 0x2B, 0x04,
            2, 0x2C, 0x01,
            2, 0x30, 0x02,
            2, 0x01, 0x22,
            2, 0x03, 0x12,
            2, 0x04, 0x00,
            2, 0x05, 0x64,
            2, 0x0A, 0x08,
            12, 0x0B, 0x0A, 0x1A, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x08, 0x1F, 0x1D,
            12, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x0D, 0x16, 0x1B, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x09, 0x1E, 0x1C,
            12, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x0F, 0x16, 0x1B, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1C, 0x1E, 0x09, 0x07,
            12, 0x10, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x11, 0x0A, 0x1A, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1D, 0x1F, 0x08, 0x06,
            12, 0x12, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            5, 0x14, 0x00, 0x00, 0x11, 0x11,
            2, 0x18, 0x99,
            2, 0x30, 0x06,
            15, 0x12, 0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29,
            15, 0x13, 0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29,
            2, 0x30, 0x0A,
            2, 0x02, 0x4F,
            2, 0x0B, 0x40,
            2, 0x12, 0x3E,
            2, 0x13, 0x78,
            2, 0x30, 0x0D,
            2, 0x0D, 0x04,
            2, 0x10, 0x0C,
            2, 0x11, 0x0C,
            2, 0x12, 0x0C,
            2, 0x13, 0x0C,
            2, 0x30, 0x00,
            2, 0x3A, 0x55,
            2, 0x34, 0x01,
            2, 0x35, 0x00,
            0
        };

        static constexpr uint8_t list1[] = {
            1, 0x11,
            0
        };

        static constexpr uint8_t list2[] = {
            1, 0x29,
            0
        };

        switch (listno) {
        case 0: return list0;
        case 1: return list1;
        case 2: return list2;
        default: return nullptr;
        }
    }

    size_t getInitDelay(size_t listno) const override
    {
        switch (listno) {
        case 1: return 120;
        case 2: return 50;
        default: return 0;
        }
    }

    bool init(bool use_reset = true) override
    {
        if (_backlight_pin >= 0) {
            ::pinMode(_backlight_pin, OUTPUT);
            ::digitalWrite(_backlight_pin, HIGH);
            delay(10);
        }
        return Panel_DSI::init(use_reset);
    }

private:
    int8_t _backlight_pin = -1;
};

} // namespace v1
} // namespace lgfx

class LGFX_JD9165 : public lgfx::LGFX_Device
{
public:
    LGFX_JD9165();

private:
    lgfx::Bus_DSI _bus;
    lgfx::Panel_JD9165_Exact _panel;
    lgfx::Touch_GT911 _touch;
};

inline LGFX_JD9165::LGFX_JD9165()
{
    auto cfg = _bus.config();
    cfg.lane_mbps = 800;
    cfg.lane_num = 2; // 2 lanes @ 800 Mbps = 1.6 Gbps total
    cfg.bus_id = 0;
    cfg.ldo_voltage_mv = 2500;
    cfg.ldo_chan_id = 3;
    cfg.lcd_cmd_bits = 8;
    cfg.lcd_param_bits = 8;
    _bus.config(cfg);
    _panel.setBus(&_bus);

    auto tcfg = _touch.config();
    tcfg.i2c_port = 1;
    tcfg.pin_sda = TP_I2C_SDA;
    tcfg.pin_scl = TP_I2C_SCL;
    tcfg.pin_rst = TP_RST;
    tcfg.pin_int = TP_INT;
    tcfg.freq = 100000;
    tcfg.x_min = 0;
    tcfg.x_max = LCD_H_RES - 1;
    tcfg.y_min = 0;
    tcfg.y_max = LCD_V_RES - 1;
    tcfg.bus_shared = true;
    _touch.config(tcfg);
    _panel.setTouch(&_touch);

    setPanel(&_panel);
}
