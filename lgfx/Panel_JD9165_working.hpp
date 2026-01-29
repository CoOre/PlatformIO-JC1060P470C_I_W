// Panel_JD9165_working.hpp - CORRECTED VERSION
#pragma once

#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>

#if SOC_MIPI_DSI_SUPPORTED

namespace lgfx
{
inline namespace v1
{

class Panel_JD9165 : public Panel_DSI
{
private:
    bool _update_enabled;

public:
    Panel_JD9165() : _update_enabled(false)
    {
        Serial.println("[Panel_JD9165] Constructor");
        
        // Configuratie
        auto cfg = config();
        cfg.panel_width = 1024;
        cfg.panel_height = 600;
        cfg.memory_width = 1024;
        cfg.memory_height = 600;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        cfg.rgb_order = false;  // BGR mode for JD9165
        cfg.invert = false;
        cfg.pin_rst = 27;
        config(cfg);
        
        // Timing
        auto detail = config_detail();
        detail.dpi_freq_mhz = 48;
        detail.hsync_pulse_width = 40;
        detail.hsync_back_porch = 160;
        detail.hsync_front_porch = 160;
        detail.vsync_pulse_width = 10;
        detail.vsync_back_porch = 23;
        detail.vsync_front_porch = 12;
        config_detail(detail);
    }
    
    // Begin functie voor initialisatie
    bool begin(bool use_reset = true)
    {
        Serial.println("[Panel_JD9165] begin() called");
        
        // Panel initialiseren via parent class
        bool result = this->init(use_reset);
        
        if (result) {
            _update_enabled = true;
            Serial.printf("[Panel_JD9165] Init success, buffer: %p\n", 
                         _config_detail.buffer);
        } else {
            Serial.println("[Panel_JD9165] Init failed!");
        }
        
        return result;
    }
    
    // Framebuffer access
    void* getFrameBuffer() const 
    { 
        return _config_detail.buffer; 
    }
    
    uint32_t getBufferSize() const 
    { 
        return _config_detail.buffer_length; 
    }
    
    // Update display
    bool updateDisplay()
    {
        if (!_update_enabled || !_config_detail.buffer) {
            return false;
        }
        
        // Gebruik de display() functie van Panel_FrameBufferBase
        this->display(0, 0, _cfg.panel_width, _cfg.panel_height);
        return true;
    }
    
    // Helper voor het vullen van buffer
    void fillBuffer(uint16_t color)
    {
        if (!_config_detail.buffer) return;
        
        uint16_t* buffer = (uint16_t*)_config_detail.buffer;
        size_t pixelCount = (_cfg.panel_width * _cfg.panel_height);
        
        for (size_t i = 0; i < pixelCount; i++) {
            buffer[i] = color;
        }
    }

protected:
    const uint8_t* getInitParams(size_t listno) const override
    {
        if (listno > 0) return nullptr;
        
        // EXACTE Arduino_GFX init sequence voor JD9165
        static constexpr uint8_t init_sequence[] = {
            // START OF WORKING ARDUINO_GFX SEQUENCE
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
            
            // Gamma tables
            12, 0x0B, 0x0A, 0x1A, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x06, 0x08, 0x1F, 0x1D,
            12, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x0D, 0x16, 0x1B, 0x0B, 0x0D, 0x0D, 0x11, 0x10, 0x07, 0x09, 0x1E, 0x1C,
            12, 0x0E, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x0F, 0x16, 0x1B, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1C, 0x1E, 0x09, 0x07,
            12, 0x10, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            12, 0x11, 0x0A, 0x1A, 0x0D, 0x0B, 0x0D, 0x11, 0x10, 0x1D, 0x1F, 0x08, 0x06,
            12, 0x12, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
            
            // Vervolg
            5, 0x14, 0x00, 0x00, 0x11, 0x11,
            2, 0x18, 0x99,
            2, 0x30, 0x06,
            
            // Color matrix
            15, 0x12, 0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29,
            15, 0x13, 0x36, 0x2C, 0x2E, 0x3C, 0x38, 0x35, 0x35, 0x32, 0x2E, 0x1D, 0x2B, 0x21, 0x16, 0x29,
            
            // Power control
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
            
            // Pixel format
            2, 0x3A, 0x55,
            
            // TE config
            2, 0x34, 0x01,
            2, 0x35, 0x00,
            
            // Sleep out
            1, 0x11,
            0x80, 120,  // Delay 120ms
            
            // Display on
            1, 0x29,
            0x80, 20,   // Delay 20ms
            
            // END
            0
        };
        
        return init_sequence;
    }
};

} // namespace v1
} // namespace lgfx

#endif // SOC_MIPI_DSI_SUPPORTED