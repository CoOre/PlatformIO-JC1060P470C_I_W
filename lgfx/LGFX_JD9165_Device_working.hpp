// LGFX_JD9165_Device_working.hpp - FIXED VERSION
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "Panel_JD9165_working.hpp"

class LGFX_JD9165 : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_DSI _bus_dsi;
    lgfx::Panel_JD9165 _panel_instance;

    LGFX_JD9165(void)
    {
        Serial.println("[LGFX_JD9165] Constructor");
        
        // Configure DSI bus
        {
            auto cfg = _bus_dsi.config();
            cfg.bus_id = 0;
            cfg.lane_num = 2;
            cfg.lane_mbps = 750;
            cfg.ldo_chan_id = 3;
            cfg.ldo_voltage_mv = 2500;
            cfg.lcd_cmd_bits = 8;
            cfg.lcd_param_bits = 8;
            _bus_dsi.config(cfg);
        }
        
        // Link panel to bus
        _panel_instance.setBus(&_bus_dsi);
        
        // Set panel to LGFX device
        setPanel(&_panel_instance);
        
        Serial.println("[LGFX_JD9165] Setup complete");
    }
    
    // Eenvoudige init functie
    bool begin(bool use_reset = true)
    {
        Serial.println("[LGFX_JD9165] begin() called");
        
        // Backlight aanzetten
        ::pinMode(23, OUTPUT);
        ::digitalWrite(23, HIGH);
        delay(100);
        
        // Reset panel als nodig
        if (use_reset) {
            ::pinMode(27, OUTPUT);
            ::digitalWrite(27, LOW);
            delay(100);
            ::digitalWrite(27, HIGH);
            delay(200);
        }
        
        // Roep de panel's begin() functie aan
        if (!_panel_instance.begin(use_reset)) {
            Serial.println("[LGFX_JD9165] Panel begin() failed!");
            return false;
        }
        
        Serial.println("[LGFX_JD9165] Panel initialized successfully");
        
        // Configureer LGFX basis
        setRotation(0);
        setColorDepth(16);
        
        Serial.println("[LGFX_JD9165] Initialization complete!");
        Serial.printf("  Resolution: %dx%d\n", width(), height());
        
        // Check framebuffer
        void* fb = getFrameBuffer();
        if (fb) {
            Serial.printf("  Framebuffer: %p (%u bytes)\n", 
                         fb, _panel_instance.getBufferSize());
        } else {
            Serial.println("  WARNING: No framebuffer!");
        }
        
        return true;
    }
    
    // Backlight control
    void setBrightness(uint8_t brightness)
    {
        analogWrite(23, brightness);
    }
    
    // Direct framebuffer access
    void* getFrameBuffer()
    {
        return _panel_instance.getFrameBuffer();
    }
    
    // Force display update
    void updateDisplay()
    {
        _panel_instance.updateDisplay();
    }
    
    // Helper voor direct tekenen
    void fillScreen(uint16_t color)
    {
        // Gebruik panel's fillBuffer() functie
        _panel_instance.fillBuffer(color);
        updateDisplay();
    }
};