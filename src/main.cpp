#include <Arduino.h>
#include "lvgl.h"
#include "pins_config.h"
#include "lcd/lgfx_jd9165.h"
#include "touch/gt911_touch.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_lcd_mipi_dsi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

LGFX_JD9165 lcd;
gt911_touch touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

lv_display_t *disp_drv;
static lv_color_t *buf;
static lv_color_t *buf1;
// Full-frame double buffer to minimize LVGL redraw overhead on large objects
static constexpr uint32_t DRAW_BUF_LINES = LCD_V_RES;
static bool use_direct_render = false;
static SemaphoreHandle_t vsync_sem = nullptr;
static bool vsync_wait_enabled = false;

static lv_indev_t *touch_indev = nullptr;
static bool lvgl_owns_touch_read = false;
static lv_obj_t *main_page = nullptr;
static lv_obj_t *status_notch = nullptr;
static lv_obj_t *time_label = nullptr;
static lv_obj_t *wifi_label = nullptr;
static lv_obj_t *battery_label = nullptr;
static lv_obj_t *fps_container = nullptr; // scrollable container for FPS test
static lv_timer_t *auto_scroll_timer = nullptr;
static bool auto_scroll_paused = false;
static bool touch_active = false;
static uint32_t last_status_ms = 0;
static uint8_t demo_battery_level = 82;
static int8_t demo_battery_delta = -1;
static bool demo_wifi_connected = true;
static volatile uint32_t last_flush_time_us = 0;
static volatile uint32_t max_flush_time_us = 0;
static volatile uint32_t flush_sample_count = 0;
static volatile uint64_t flush_time_accum_us = 0;

struct touch_calibration_t {
    int16_t x_min;
    int16_t x_max;
    int16_t y_min;
    int16_t y_max;
};

// Raw touch area seems larger than visible LCD; adjust min/max instead of magic offsets
static touch_calibration_t touch_cal = {
    .x_min = 0,    // update after checking raw logs
    .x_max = 800,  // measured far corner raw X
    .y_min = 0,
    .y_max = 480,  // measured far corner raw Y
};

static lv_coord_t map_touch(int16_t raw, int16_t min_raw, int16_t max_raw, lv_coord_t max_out)
{
    int32_t span = static_cast<int32_t>(max_raw) - static_cast<int32_t>(min_raw);
    if (span <= 0) return 0;
    int32_t val = (static_cast<int32_t>(raw) - static_cast<int32_t>(min_raw)) * max_out / span;
    if (val < 0) val = 0;
    if (val > max_out) val = max_out;
    return static_cast<lv_coord_t>(val);
}

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_map)
{
    if (!use_direct_render) {
        const int offsetx1 = area->x1;
        const int offsetx2 = area->x2;
        const int offsety1 = area->y1;
        const int offsety2 = area->y2;
        const int32_t w = offsetx2 - offsetx1 + 1;
        const int32_t h = offsety2 - offsety1 + 1;
        lcd.startWrite();
        lcd.pushImage(offsetx1, offsety1, w, h, reinterpret_cast<const lgfx::rgb565_t *>(color_map));
        lcd.endWrite();
    }
    if (use_direct_render && vsync_wait_enabled && vsync_sem) {
        (void)xSemaphoreTake(vsync_sem, pdMS_TO_TICKS(35));
    }
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
    bool touched;
    uint16_t touchX, touchY;

    touched = touch.getTouch(&touchX, &touchY);
    touch_active = touched;

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = map_touch(touchX, touch_cal.x_min, touch_cal.x_max, LCD_H_RES - 1);
        data->point.y = map_touch(touchY, touch_cal.y_min, touch_cal.y_max, LCD_V_RES - 1);
    }
}

static void set_label_palette(lv_obj_t *obj)
{
    lv_obj_set_style_text_color(obj, lv_color_hex(0xE6EDF7), LV_PART_MAIN);
}

static const char *battery_icon_for_level(uint8_t level, bool charging)
{
    if (charging) return LV_SYMBOL_CHARGE;
    if (level > 85) return LV_SYMBOL_BATTERY_FULL;
    if (level > 65) return LV_SYMBOL_BATTERY_3;
    if (level > 45) return LV_SYMBOL_BATTERY_2;
    if (level > 25) return LV_SYMBOL_BATTERY_1;
    return LV_SYMBOL_BATTERY_EMPTY;
}

static void update_time_label(uint32_t now_ms)
{
    if (!time_label) return;
    uint32_t total_seconds = now_ms / 1000;
    uint32_t hours = (total_seconds / 3600U) % 24U;
    uint32_t minutes = (total_seconds / 60U) % 60U;
    uint32_t seconds = total_seconds % 60U;
    lv_label_set_text_fmt(time_label, "%02u:%02u:%02u",
                          static_cast<unsigned>(hours),
                          static_cast<unsigned>(minutes),
                          static_cast<unsigned>(seconds));
}

static void refresh_status_bar(bool wifi_ok, uint8_t battery_level, bool charging, uint32_t now_ms)
{
    if (!status_notch) return;
    lv_label_set_text(wifi_label, LV_SYMBOL_WIFI);
    const char *battery_icon = battery_icon_for_level(battery_level, charging);
    lv_label_set_text_fmt(battery_label, "%s %u%%", battery_icon, static_cast<unsigned>(battery_level));
    update_time_label(now_ms);
}

static void create_main_page()
{
    main_page = lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_page, LCD_H_RES, LCD_V_RES);
    lv_obj_align(main_page, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(main_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(main_page, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(main_page, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(main_page, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_page, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_page, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(main_page, 0, LV_PART_MAIN);
    lv_obj_set_layout(main_page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_page, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
}

static void create_status_notch()
{
    status_notch = lv_obj_create(main_page);
    lv_obj_set_width(status_notch, LV_PCT(100));
    lv_obj_set_height(status_notch, 28);
    lv_obj_set_style_bg_color(status_notch, lv_color_hex(0x0C101B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(status_notch, lv_color_hex(0x141C2B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(status_notch, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(status_notch, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(status_notch, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_notch, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(status_notch, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(status_notch, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(status_notch, 8, LV_PART_MAIN);
    lv_obj_set_layout(status_notch, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_notch, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_notch, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(status_notch, 12, LV_PART_MAIN);

    time_label = lv_label_create(status_notch);
    set_label_palette(time_label);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(time_label, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_flex_grow(time_label, 1);

    lv_obj_t *status_right_box = lv_obj_create(status_notch);
    lv_obj_set_size(status_right_box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(status_right_box, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_right_box, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_right_box, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_bg_opa(status_right_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_right_box, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(status_right_box, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(status_right_box, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_right_box, 0, LV_PART_MAIN);
    lv_obj_set_style_min_width(status_right_box, 100, LV_PART_MAIN);

    wifi_label = lv_label_create(status_right_box);
    set_label_palette(wifi_label);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, LV_PART_MAIN);
    
    battery_label = lv_label_create(status_right_box);
    set_label_palette(battery_label);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_14, LV_PART_MAIN);

}

static void create_fps_test_list()
{
    fps_container = lv_obj_create(main_page);
    lv_obj_set_width(fps_container, LV_PCT(100));
    lv_obj_set_flex_grow(fps_container, 1);
    lv_obj_set_scroll_dir(fps_container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(fps_container, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_all(fps_container, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(fps_container, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(fps_container, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fps_container, LV_OPA_TRANSP, LV_PART_MAIN);

    // Static scene: 1000px tall content with scattered squares.
    lv_obj_t *content = lv_obj_create(fps_container);
    lv_obj_set_size(content, LV_PCT(100), 1000);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);

    const lv_coord_t max_w = LCD_H_RES - 16; // account for padding
    const lv_coord_t max_h = 1000;
    const uint32_t count = 80;

    for (uint32_t i = 0; i < count; ++i) {
        const lv_coord_t w = 40 + (i * 13u % 120);
        const lv_coord_t h = 30 + (i * 17u % 100);
        const lv_coord_t x = (i * 37u) % ((max_w > w) ? (max_w - w) : 1);
        const lv_coord_t y = (i * 53u) % ((max_h > h) ? (max_h - h) : 1);

        lv_obj_t *box = lv_obj_create(content);
        lv_obj_set_size(box, w, h);
        lv_obj_set_pos(box, x, y);
        lv_obj_set_style_radius(box, 6, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);

        const uint32_t color = ((i * 97u) % 255) << 16 | ((i * 57u) % 255) << 8 | ((i * 23u) % 255);
        lv_obj_set_style_bg_color(box, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(box);
        lv_label_set_text_fmt(label, "#%02u", static_cast<unsigned>(i));
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    }

    // Auto-scroll timer (paused while touching the screen).
    auto_scroll_timer = lv_timer_create(
        [](lv_timer_t *t) {
            // LV_UNUSED(t);
            // if (!fps_container) return;
            // static bool direction_down = true;
            // const int32_t step = 2;
            // const int32_t can_scroll_down = lv_obj_get_scroll_bottom(fps_container);
            // const int32_t can_scroll_up = lv_obj_get_scroll_top(fps_container);
            // if (direction_down && can_scroll_down <= 0) {
            //     direction_down = false;
            // } else if (!direction_down && can_scroll_up <= 0) {
            //     direction_down = true;
            // }
            // const int32_t dy = direction_down ? -step : step; // negative dy scrolls down
            // lv_obj_scroll_by_bounded(fps_container, 0, dy, LV_ANIM_OFF);
        },
        16,
        nullptr);
}

static void animate_status_bar(uint32_t now_ms)
{
    if (demo_battery_level <= 20) demo_battery_delta = 1;
    else if (demo_battery_level >= 96) demo_battery_delta = -1;
    demo_battery_level = static_cast<uint8_t>(demo_battery_level + demo_battery_delta);
    if ((now_ms / 7000U) % 2U == 0U) {
        demo_wifi_connected = true;
    } else {
        demo_wifi_connected = false;
    }
    bool charging = demo_battery_delta > 0;
    refresh_status_bar(demo_wifi_connected, demo_battery_level, charging, now_ms);
}

void setup()
{
    // Use UART0 for logs (visible on the same port as ROM boot messages)
    Serial0.begin(115200);
    Serial.begin(115200); // USB-CDC (if available)
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("GT911", ESP_LOG_DEBUG);
    Serial0.println("ESP32P4 MIPI DSI LVGL (UART0)");
    Serial.println("ESP32P4 MIPI DSI LVGL");

    lcd.setColorDepth(16);
    lcd.setSwapBytes(true); // RGB565 from LVGL usually needs byte swap on ESP
    bool lcd_ok = lcd.init();
    if (!lcd_ok) {
        Serial0.println("LGFX init failed");
        Serial.println("LGFX init failed");
    }
    lcd.initDMA();
    pinMode(LCD_LED, OUTPUT);
    digitalWrite(LCD_LED, HIGH);
    touch.begin(); // enable touch, but do not wire into LVGL yet

    lv_init();
    const uint32_t buffer_pixels = LCD_H_RES * DRAW_BUF_LINES;
    const size_t buf_bytes = buffer_pixels * sizeof(lv_color_t);

    disp_drv = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp_drv, my_disp_flush);
    void *fb0 = nullptr;
    void *fb1 = nullptr;
    if (lcd.getFrameBuffers(&fb0, &fb1)) {
        buf = static_cast<lv_color_t *>(fb0);
        buf1 = static_cast<lv_color_t *>(fb1);
        use_direct_render = true;
        const size_t full_buf_bytes = static_cast<size_t>(LCD_H_RES) * LCD_V_RES * sizeof(lv_color_t);
        lv_display_set_buffers(disp_drv, buf, buf1, full_buf_bytes, LV_DISPLAY_RENDER_MODE_DIRECT);
        Serial0.printf("LVGL direct FB enabled: fb0=%p fb1=%p\n", buf, buf1);
        Serial.printf("LVGL direct FB enabled: fb0=%p fb1=%p\n", buf, buf1);
        esp_lcd_dpi_panel_event_callbacks_t cbs = {};
        cbs.on_refresh_done = on_dpi_refresh_done;
        if (!lcd.registerDpiCallbacks(&cbs, disp_drv)) {
            Serial0.println("VSYNC event registration failed.");
            Serial.println("VSYNC event registration failed.");
        } else {
            vsync_sem = xSemaphoreCreateBinary();
            if (vsync_sem) {
                vsync_wait_enabled = true;
                Serial0.println("VSYNC wait enabled (semaphore).");
                Serial.println("VSYNC wait enabled (semaphore).");
            }
        }
    } else {
        // Fallback: Full-screen buffers live in PSRAM; DMA capable is required for the driver copy
        buf = static_cast<lv_color_t *>(heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM));
        buf1 = static_cast<lv_color_t *>(heap_caps_malloc(buf_bytes, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM));
        assert(buf);
        assert(buf1);
        lv_display_set_buffers(disp_drv, buf, buf1, buf_bytes, LV_DISPLAY_RENDER_MODE_FULL);
        Serial0.println("LVGL direct FB not available; using FULL render.");
        Serial.println("LVGL direct FB not available; using FULL render.");
    }

    // Enable LVGL input device for touch
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touchpad_read);
    lv_indev_set_display(touch_indev, disp_drv);
    lvgl_owns_touch_read = true; // avoid double-polling from loop()

    create_main_page();
    create_status_notch();
    refresh_status_bar(demo_wifi_connected, demo_battery_level, false, millis());
    create_fps_test_list();

    Serial.println("setup ");
}

void loop()
{
    static uint32_t last_ms = 0;
    uint32_t now = millis();
    uint32_t elapsed = (last_ms == 0) ? 0 : (now - last_ms);
    last_ms = now;
    uint16_t x, y;
    if (elapsed > 0) {
        lv_tick_inc(elapsed); // advance LVGL tick for timers/indev
    }
    if (now - last_status_ms >= 1000) {
        last_status_ms = now;
        animate_status_bar(now);
    }
    static uint32_t last_flush_report_ms = 0;
    if (now - last_flush_report_ms >= 2000 && flush_sample_count > 0) {
        const uint32_t samples = flush_sample_count;
        const uint64_t accum = flush_time_accum_us;
        const uint32_t max_us = max_flush_time_us;
        flush_sample_count = 0;
        flush_time_accum_us = 0;
        max_flush_time_us = 0;
        last_flush_report_ms = now;
        const uint32_t avg_us = samples ? static_cast<uint32_t>(accum / samples) : 0U;
    }
    lv_timer_handler();
    if (auto_scroll_timer) {
        if (touch_active && !auto_scroll_paused) {
            lv_timer_pause(auto_scroll_timer);
            auto_scroll_paused = true;
        } else if (!touch_active && auto_scroll_paused) {
            lv_timer_resume(auto_scroll_timer);
            auto_scroll_paused = false;
        }
    }
    delay(10); // small yield; adjust if WiFi/tasks need time
}
