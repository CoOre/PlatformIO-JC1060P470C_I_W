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

static_assert(LV_COLOR_DEPTH == 16, "LV_COLOR_DEPTH must be 16 for RGB565 panel");

LGFX_JD9165 lcd;

lv_display_t *disp_drv;
static lv_color_t *buf;
static lv_color_t *buf1;
static constexpr uint32_t DRAW_BUF_LINES = 100; // 100*1024*2 = 205KB - fits in SRAM
static SemaphoreHandle_t vsync_sem = nullptr;

static lv_indev_t *touch_indev = nullptr;

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
    lcd.pushImage(offsetx1, offsety1, w, h, reinterpret_cast<const lgfx::rgb565_t *>(color_map));
    lcd.endWrite();
    
    lv_display_flush_ready(disp);
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

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
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
    static uint32_t last_ms = 0;
    uint32_t now = millis();
    uint32_t elapsed = (last_ms == 0) ? 0 : (now - last_ms);
    last_ms = now;
    
    if (elapsed > 0) {
        lv_tick_inc(elapsed);
    }
    
    // Update UI manager
    ui::UIManager::instance().update(now);
    
    lv_timer_handler();
    update_fps_counter();
}
