#pragma once

#define LGFX_USE_V1
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>
#include <esp_err.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_mipi_dsi.h>
#include "pins_config.h"

namespace lgfx
{
inline namespace v1
{

// JD9165 panel definition matching the init sequence shared in LovyanGFX issue #803
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
        cfg.rgb_order = false;
        cfg.offset_rotation = 0;
        config(cfg);

        auto detail = config_detail();
        detail.dpi_freq_mhz = 42; // closer to 60Hz timing; verify panel stability
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
            2, 0x3A, 0x55, // RGB565 pixel format
            // TE enable (tearing effect sync) like Arduino_GFX working example
            2, 0x34, 0x01,
            2, 0x35, 0x00,
            0 // end of list
        };

        static constexpr uint8_t list1[] = {
            1, 0x11, // SLPOUT, no params
            0
        };

        static constexpr uint8_t list2[] = {
            1, 0x29, // DISPON, no params
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
        case 1: return 120; // after SLPOUT
        case 2: return 50;  // after DISPON
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

    void *getFrameBufferPtr() const { return _config_detail.buffer; }
    bool registerDpiCallbacks(const esp_lcd_dpi_panel_event_callbacks_t *cbs, void *user_ctx) const
    {
        if (!_disp_panel_handle || !cbs) return false;
        return esp_lcd_dpi_panel_register_event_callbacks(_disp_panel_handle, cbs, user_ctx) == ESP_OK;
    }

    bool getFrameBuffers(void **out_fb0, void **out_fb1) const
    {
        if (!out_fb0 || !out_fb1 || !_disp_panel_handle) return false;
        void *fb0 = nullptr;
        void *fb1 = nullptr;
        esp_err_t err = esp_lcd_dpi_panel_get_frame_buffer(_disp_panel_handle, 2, &fb0, &fb1);
        if (err == ESP_ERR_INVALID_ARG) {
            err = esp_lcd_dpi_panel_get_frame_buffer(_disp_panel_handle, 1, &fb0);
        }
        if (err != ESP_OK || !fb0) return false;
        *out_fb0 = fb0;
        *out_fb1 = fb1;
        return true;
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
    void *getFrameBufferPtr() const { return _panel.getFrameBufferPtr(); }
    bool registerDpiCallbacks(const esp_lcd_dpi_panel_event_callbacks_t *cbs, void *user_ctx) const
    {
        return _panel.registerDpiCallbacks(cbs, user_ctx);
    }
    bool getFrameBuffers(void **out_fb0, void **out_fb1) const { return _panel.getFrameBuffers(out_fb0, out_fb1); }

private:
    lgfx::Bus_DSI _bus;
    lgfx::Panel_JD9165_Exact _panel;
};

inline LGFX_JD9165::LGFX_JD9165()
{
    auto cfg = _bus.config();
    cfg.lane_mbps = 750; // align with proven Arduino_GFX working config
    cfg.lane_num = 2;
    cfg.bus_id = 0;
    cfg.ldo_voltage_mv = 2500;
    cfg.ldo_chan_id = 3;
    cfg.lcd_cmd_bits = 8;
    cfg.lcd_param_bits = 8;
    _bus.config(cfg);
    _panel.setBus(&_bus);
    setPanel(&_panel);
}
