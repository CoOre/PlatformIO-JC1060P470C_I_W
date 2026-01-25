#include <Arduino.h>
#include "lvgl.h"
#include "pins_config.h"
#include "lcd/jd9165_lcd.h"
#include "touch/gt911_touch.h"
#include "esp_heap_caps.h"

jd9165_lcd lcd(LCD_RST);
gt911_touch touch(TP_I2C_SDA, TP_I2C_SCL, TP_RST, TP_INT);

lv_display_t *disp_drv;
static lv_color_t *buf;
static lv_color_t *buf1;

static lv_indev_t *touch_indev = nullptr;
static bool lvgl_owns_touch_read = false;
static lv_obj_t *main_page = nullptr;
static lv_obj_t *status_notch = nullptr;
static lv_obj_t *time_label = nullptr;
static lv_obj_t *wifi_label = nullptr;
static lv_obj_t *battery_label = nullptr;
static lv_obj_t *fps_list = nullptr; // scrollable container for FPS test
static lv_obj_t *fps_label = nullptr;
static uint32_t last_status_ms = 0;
static uint8_t demo_battery_level = 82;
static int8_t demo_battery_delta = -1;
static bool demo_wifi_connected = true;
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
    const int offsetx1 = area->x1;
    const int offsetx2 = area->x2;
    const int offsety1 = area->y1;
    const int offsety2 = area->y2;
    lcd.lcd_draw_bitmap(offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    lv_display_flush_ready(disp);
}

static void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
    bool touched;
    uint16_t touchX, touchY;

    touched = touch.getTouch(&touchX, &touchY);

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
    // Place a long text list in the main area to stress scrolling/fps with minimal objects.
    fps_list = lv_obj_create(main_page);
    lv_obj_set_width(fps_list, LV_PCT(100));
    lv_obj_set_flex_grow(fps_list, 1); // take remaining height in column layout
    lv_obj_set_scroll_dir(fps_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(fps_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_pad_all(fps_list, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(fps_list, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(fps_list, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(fps_list, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(fps_list, 0, LV_PART_MAIN);

    fps_label = lv_label_create(fps_list);
    lv_obj_set_width(fps_label, LV_PCT(100));
    lv_obj_set_style_pad_all(fps_label, 5, LV_PART_MAIN);
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0x111111), LV_PART_MAIN);
    lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(fps_label, 6, LV_PART_MAIN);
    lv_label_set_long_mode(fps_label, LV_LABEL_LONG_WRAP);

    static char list_text[8192];
    size_t offset = 0;
    const int item_count = 1000;
    for (int i = 1; i <= item_count; ++i) {
        int written = lv_snprintf(list_text + offset, sizeof(list_text) - offset, "Line %03d - fps test", i);
        if (written <= 0) break;
        offset += static_cast<size_t>(written);
        if (offset >= sizeof(list_text) - 1) break;
    }
    lv_label_set_text(fps_label, list_text);
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

    lcd.begin();
    touch.begin(); // enable touch, but do not wire into LVGL yet

    lv_init();
    uint32_t buffer_size = LCD_H_RES * LCD_V_RES;

    // Allocate full-size double buffer (LV_COLOR_DEPTH=16 -> 2 bytes per pixel)
    size_t buf_bytes = buffer_size * sizeof(lv_color_t);
    buf = static_cast<lv_color_t *>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM));
    buf1 = static_cast<lv_color_t *>(heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM));
    assert(buf);
    assert(buf1);

    disp_drv = lv_display_create(LCD_H_RES, LCD_V_RES);
    lv_display_set_flush_cb(disp_drv, my_disp_flush);
    lv_display_set_buffers(disp_drv, buf, buf1, buffer_size * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_FULL);

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
    if (!lvgl_owns_touch_read && touch.getTouch(&x, &y)) {
        Serial0.printf("TOUCH: x=%u y=%u\r\n", x, y);
        Serial.printf("TOUCH: x=%u y=%u\r\n", x, y);
    }
    if (elapsed > 0) {
        lv_tick_inc(elapsed); // advance LVGL tick for timers/indev
    }
    if (now - last_status_ms >= 1000) {
        last_status_ms = now;
        animate_status_bar(now);
    }
    lv_timer_handler();
    delay(5);
}
