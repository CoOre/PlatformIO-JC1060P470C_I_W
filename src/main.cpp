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

static_assert(LV_COLOR_DEPTH == 16, "LV_COLOR_DEPTH must be 16 for RGB565 panel");

LGFX_JD9165 lcd;

lv_display_t *disp_drv;
static lv_color_t *buf;
static lv_color_t *buf1;
static constexpr uint32_t DRAW_BUF_LINES = 100; // 100*1024*2 = 205KB - fits in SRAM
static SemaphoreHandle_t vsync_sem = nullptr;

static lv_indev_t *touch_indev = nullptr;
static lv_obj_t *main_page = nullptr;
static lv_obj_t *status_notch = nullptr;
static lv_obj_t *time_label = nullptr;
static lv_obj_t *wifi_label = nullptr;
static lv_obj_t *battery_label = nullptr;
static lv_obj_t *fps_container = nullptr;
static lv_timer_t *auto_scroll_timer = nullptr;
static bool auto_scroll_paused = false;
static bool touch_active = false;
static uint32_t last_status_ms = 0;
static uint8_t demo_battery_level = 82;
static int8_t demo_battery_delta = -1;
static bool demo_wifi_connected = true;

// FPS counter
static lv_obj_t *fps_label = nullptr;
static uint32_t fps_frame_count = 0;
static uint32_t fps_last_time = 0;
static uint32_t fps_current = 0;

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
    touch_active = touched;

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
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

    lv_obj_t *content = lv_obj_create(fps_container);
    lv_obj_set_size(content, LV_PCT(100), 1000);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);

    const lv_coord_t max_w = LCD_H_RES - 16;
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

    auto_scroll_timer = lv_timer_create([](lv_timer_t *t) { }, 16, nullptr);
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

    create_main_page();
    create_status_notch();
    refresh_status_bar(demo_wifi_connected, demo_battery_level, false, millis());
    create_fps_test_list();
    
    // FPS label
    fps_label = lv_label_create(lv_layer_top());
    lv_obj_set_pos(fps_label, 10, 40);
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(fps_label, "FPS: --");

    Serial.println("setup done");
}

static void update_fps_counter()
{
    fps_frame_count++;
    uint32_t now = millis();
    if (now - fps_last_time >= 1000) {
        fps_current = fps_frame_count;
        fps_frame_count = 0;
        fps_last_time = now;
        if (fps_label) {
            lv_label_set_text_fmt(fps_label, "FPS: %u", fps_current);
        }
    }
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
    
    if (now - last_status_ms >= 1000) {
        last_status_ms = now;
        animate_status_bar(now);
    }
    
    lv_timer_handler();
    update_fps_counter();
    
    if (auto_scroll_timer) {
        if (touch_active && !auto_scroll_paused) {
            lv_timer_pause(auto_scroll_timer);
            auto_scroll_paused = true;
        } else if (!touch_active && auto_scroll_paused) {
            lv_timer_resume(auto_scroll_timer);
            auto_scroll_paused = false;
        }
    }
}
