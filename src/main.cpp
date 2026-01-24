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

static void btn_event_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_t *label = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
        lv_label_set_text(label, "Clicked!");
    }
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

    // Simple UI: greeting label and a button
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello, LVGL!");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *btn = lv_btn_create(lv_screen_active());
    lv_obj_center(btn);
    lv_obj_t *btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Press me");
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, btn_label);

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
    lv_timer_handler();
    delay(5);
}
