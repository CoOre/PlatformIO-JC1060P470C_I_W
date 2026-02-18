#include <Arduino.h>
#include "lvgl.h"
#include "pins_config.h"
#include "lcd/lgfx_jd9165.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_lcd_mipi_dsi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "ui/ui_manager.h"
#include "settings/settings_app.h"
#include "settings/core/settings_store.h"
#include "settings/services/display_service.h"

static_assert(LV_COLOR_DEPTH == 16, "LV_COLOR_DEPTH must be 16 for RGB565 panel");

LGFX_JD9165 lcd;

lv_display_t *disp_drv;
static lv_color_t *buf;
static lv_color_t *buf1;
static constexpr uint32_t DRAW_BUF_LINES = 100; // 100*1024*2 = 205KB - fits in SRAM
static SemaphoreHandle_t vsync_sem = nullptr;

static lv_indev_t *touch_indev = nullptr;

// Current display rotation (0=0°, 1=90°, 2=180°, 3=270°)
static uint8_t current_rotation = 0;

// Display resolution (swapped based on rotation)
static uint32_t display_hor_res = LCD_H_RES;
static uint32_t display_ver_res = LCD_V_RES;

// FPS counter
static uint32_t fps_frame_count = 0;
static uint32_t fps_last_time = 0;

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    const int32_t w = offsetx2 - offsetx1 + 1;
    const int32_t h = offsety2 - offsety1 + 1;
    const size_t pixel_size = lv_color_format_get_size(lv_display_get_color_format(disp));
    const size_t bytes = (w <= 0 || h <= 0) ? 0u : static_cast<size_t>(w) * static_cast<size_t>(h) * pixel_size;
    
    lcd.startWrite();
    
    // Handle rotation for display flush
    // LVGL handles the rotation internally, we just need to push the image
    lcd.pushImage(offsetx1, offsety1, w, h, reinterpret_cast<const lgfx::rgb565_t *>(color_map));
    
    lcd.endWrite();
    
    lv_display_flush_ready(disp);
}

/**
 * @brief Apply display and touch rotation
 * @param rotation 0=0°, 1=90°, 2=180°, 3=270°
 */
static void applyDisplayRotation(uint8_t rotation)
{
    if (!disp_drv) return;
    
    rotation = rotation % 4;
    current_rotation = rotation;
    
    // Apply LovyanGFX rotation
    lcd.setRotation(rotation);
    
    // Update display dimensions based on rotation
    if (rotation == 1 || rotation == 3) {
        // 90° or 270° - swap dimensions
        display_hor_res = LCD_V_RES;
        display_ver_res = LCD_H_RES;
    } else {
        // 0° or 180° - normal dimensions
        display_hor_res = LCD_H_RES;
        display_ver_res = LCD_V_RES;
    }

    // Keep LVGL rotation at 0 and just update the logical resolution.
    // LVGL rotates input based on display rotation, so avoid double-rotation.
    lv_display_set_rotation(disp_drv, LV_DISPLAY_ROTATION_0);
    lv_display_set_resolution(disp_drv,
                              static_cast<int32_t>(display_hor_res),
                              static_cast<int32_t>(display_ver_res));
    
    ESP_LOGI("MAIN", "Display rotation set to %d (%d\xc2\xb0)", rotation, rotation * 90);
}

/**
 * @brief Get the current display rotation
 */
static uint8_t getDisplayRotation()
{
    return current_rotation;
}

static bool on_dpi_refresh_done(esp_lcd_panel_handle_t panel,
                                esp_lcd_dpi_panel_event_data_t *edata,
                                void *user_ctx)
{
    LV_UNUSED(panel);
    LV_UNUSED(edata);
    lv_display_t *disp = static_cast<lv_display_t *>(user_ctx);
    if (disp) {
        lv_display_send_vsync_event(disp, nullptr);
    }
    BaseType_t high_task_woken = pdFALSE;
    if (vsync_sem) {
        xSemaphoreGiveFromISR(vsync_sem, &high_task_woken);
    }
    return false;
}

static void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX = 0;
    uint16_t touchY = 0;
    uint_fast8_t count = lcd.getTouch(&touchX, &touchY);
    bool touched = count > 0;
    
    // Update UI manager with touch state
    ui::UIManager::instance().setTouchActive(touched);
    
    // Reset backlight timeout on user activity
    if (touched) {
        settings::DisplayService::instance().markActivity();
    }

    // Debug: log touch coordinates (throttled)
    static uint32_t last_debug_time = 0;
    static uint16_t last_touchX = 0xFFFF;
    static uint16_t last_touchY = 0xFFFF;
    uint32_t now = millis();
    if (touched && (now - last_debug_time > 200 || touchX != last_touchX || touchY != last_touchY)) {
        // Serial0.printf("Touch: raw=(%u, %u)\r\n", touchX, touchY);
        last_debug_time = now;
        last_touchX = touchX;
        last_touchY = touchY;
    }

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        // LovyanGFX already applies rotation in getTouch() when lcd.setRotation() is used.
        // Avoid double-rotating here; just clamp to the current display bounds.
        uint16_t finalX = touchX;
        uint16_t finalY = touchY;
        uint32_t hor_res = display_hor_res;
        uint32_t ver_res = display_ver_res;

        if (finalX >= hor_res) finalX = hor_res - 1;
        if (finalY >= ver_res) finalY = ver_res - 1;

        data->point.x = finalX;
        data->point.y = finalY;
        
        #if 0
        // Apply touch coordinate transformation based on rotation
        // LovyanGFX getTouch returns coordinates in the panel's native orientation
        // We need to transform them to match LVGL's rotated coordinate system
        uint16_t finalX = touchX;
        uint16_t finalY = touchY;
        
        // Get current display dimensions
        uint32_t hor_res = display_hor_res;
        uint32_t ver_res = display_ver_res;
        
        // Transform coordinates based on rotation
        // Native panel is 1024x600 (LCD_H_RES x LCD_V_RES)
        switch (current_rotation) {
            case 0: // 0° - native orientation
                // No transformation needed
                finalX = touchX;
                finalY = touchY;
                break;
                
            case 1: // 90° clockwise
                // Panel rotated 90°: X becomes Y, Y becomes (width - X)
                // Native: X=0..1023, Y=0..599
                // Rotated: X=0..599, Y=0..1023
                finalX = touchY;
                finalY = (LCD_H_RES - 1) - touchX;
                break;
                
            case 2: // 180°
                // Panel rotated 180°: both axes inverted
                finalX = (LCD_H_RES - 1) - touchX;
                finalY = (LCD_V_RES - 1) - touchY;
                break;
                
            case 3: // 270° clockwise (or 90° counter-clockwise)
                // Panel rotated 270°: X becomes (height - Y), Y becomes X
                finalX = (LCD_V_RES - 1) - touchY;
                finalY = touchX;
                break;
        }
        
        // Clamp to display bounds
        if (finalX >= hor_res) finalX = hor_res - 1;
        if (finalY >= ver_res) finalY = ver_res - 1;
        
        data->point.x = finalX;
        data->point.y = finalY;
#endif
    }
}

static void update_fps_counter()
{
    fps_frame_count++;
    uint32_t now = millis();
    if (now - fps_last_time >= 1000) {
        uint32_t fps = fps_frame_count;
        fps_frame_count = 0;
        fps_last_time = now;
        ui::UIManager::instance().updateFPS(fps);
    }
}

void setup()
{
    Serial0.begin(115200);
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("GT911", ESP_LOG_DEBUG);
    Serial0.println("ESP32P4 MIPI DSI LVGL (UART0)");
    Serial.println("ESP32P4 MIPI DSI LVGL");

    lcd.setColorDepth(16);
    bool lcd_ok = lcd.init();
    if (!lcd_ok) {
        Serial0.println("LGFX init failed");
        Serial.println("LGFX init failed");
    }
    lcd.initDMA();
    pinMode(LCD_LED, OUTPUT);
    digitalWrite(LCD_LED, HIGH);

    lv_init();

    disp_drv = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_color_format(disp_drv, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp_drv, my_disp_flush);

    const uint32_t buffer_pixels = LCD_H_RES * DRAW_BUF_LINES;
    const size_t pixel_size = lv_color_format_get_size(lv_display_get_color_format(disp_drv));
    const size_t buf_bytes = buffer_pixels * pixel_size;
    
    Serial0.printf("LVGL color depth: %d, pixel size=%u bytes\n", LV_COLOR_DEPTH, static_cast<unsigned>(pixel_size));
    Serial.printf("LVGL color depth: %d, pixel size=%u bytes\n", LV_COLOR_DEPTH, static_cast<unsigned>(pixel_size));

    // Single buffer in SRAM for LVGL (saves memory, 3 buffers total instead of 4)
    buf = static_cast<lv_color_t *>(heap_caps_aligned_alloc(64, buf_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA));
    
    if (!buf) {
        Serial0.printf("LVGL draw buffer alloc failed (need %u bytes)\n", static_cast<unsigned>(buf_bytes));
        Serial.printf("LVGL draw buffer alloc failed (need %u bytes)\n", static_cast<unsigned>(buf_bytes));
        while (true) { delay(1000); }
    }

    lv_display_set_buffers(disp_drv, buf, NULL, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial0.printf("LVGL PARTIAL render single buffer (%u lines).\n", static_cast<unsigned>(DRAW_BUF_LINES));
    Serial.printf("LVGL PARTIAL render single buffer (%u lines).\n", static_cast<unsigned>(DRAW_BUF_LINES));
    Serial0.printf("LVGL buffer: buf=%p\n", buf);
    Serial.printf("LVGL buffer: buf=%p\n", buf);

    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touchpad_read);
    lv_indev_set_display(touch_indev, disp_drv);

    // Initialize settings services
    Serial0.println("Initializing settings services...");
    Serial.println("Initializing settings services...");
    if (!settings::initSettingsServices()) {
        Serial0.println("Failed to init settings services");
        Serial.println("Failed to init settings services");
    } else {
        Serial0.println("Settings services initialized OK");
        Serial.println("Settings services initialized OK");
        
        // Apply saved display rotation
        uint8_t saved_rotation = settings::SettingsStore::instance().getRotation();
        Serial0.printf("Applying saved rotation: %d (%d\xc2\xb0)\n", saved_rotation, saved_rotation * 90);
        Serial.printf("Applying saved rotation: %d (%d\xc2\xb0)\n", saved_rotation, saved_rotation * 90);
        applyDisplayRotation(saved_rotation);
    }

    // Initialize UI manager
    if (!ui::UIManager::instance().init()) {
        Serial0.println("UI Manager init failed");
        Serial.println("UI Manager init failed");
        while (true) { delay(1000); }
    }

    Serial.println("setup done");
}

void loop()
{
    static constexpr uint32_t TARGET_FPS = 30;
    static constexpr uint32_t FRAME_TIME_MS = 1000 / TARGET_FPS;
    static uint32_t last_ms = 0;
    uint32_t now = millis();
    uint32_t elapsed = (last_ms == 0) ? 0 : (now - last_ms);
    last_ms = now;
    
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
    }
    
    // Check for rotation changes from settings
    static uint8_t last_checked_rotation = 0xFF;
    uint8_t current_settings_rotation = settings::SettingsStore::instance().getRotation();
    if (current_settings_rotation != last_checked_rotation) {
        last_checked_rotation = current_settings_rotation;
        if (current_settings_rotation != current_rotation) {
            applyDisplayRotation(current_settings_rotation);
        }
    }
    
    // Update UI manager
    ui::UIManager::instance().update(now);
    
    lv_timer_handler();
    update_fps_counter();

    uint32_t frame_time = millis() - now;
    if (frame_time < FRAME_TIME_MS) {
        delay(FRAME_TIME_MS - frame_time);
    }
}
