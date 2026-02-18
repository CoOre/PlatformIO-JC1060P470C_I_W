#include "clock_app.h"
#include "ui_manager.h"
#include <cstdio>
#include <ctime>

namespace ui {

ClockApp::ClockApp() = default;

ClockApp::~ClockApp() {
    destroy();
}

bool ClockApp::create(lv_obj_t* parent) {
    if (!parent) return false;
    if (container_) return true;

    setupContainer(parent);
    createTimeDisplay();
    createDateDisplay();
    createSettingsPanel();

    return true;
}

void ClockApp::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        time_label_ = nullptr;
        date_label_ = nullptr;
        settings_panel_ = nullptr;
        settings_btn_ = nullptr;
    }
}

void ClockApp::setupContainer(lv_obj_t* parent) {
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));

    // Remove padding
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(container_, 0, LV_PART_MAIN);

    // Style
    lv_obj_set_style_bg_color(container_, lv_color_hex(bg_color_), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);
}

void ClockApp::createTimeDisplay() {
    // Main time label (HH:MM:SS)
    time_label_ = lv_label_create(container_);
    lv_label_set_text(time_label_, "00:00:00");
    lv_obj_set_style_text_color(time_label_, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_48, LV_PART_MAIN);
    // Position in center
    lv_obj_center(time_label_);
    // Settings button (small gear icon area)
    settings_btn_ = lv_btn_create(container_);
    lv_obj_set_size(settings_btn_, 60, 60);
    lv_obj_align(settings_btn_, LV_ALIGN_TOP_RIGHT, -20, 20);
    lv_obj_set_style_bg_color(settings_btn_, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settings_btn_, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(settings_btn_, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_btn_, 0, LV_PART_MAIN);

    lv_obj_t* settings_label = lv_label_create(settings_btn_);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(settings_label, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_center(settings_label);

    lv_obj_add_event_cb(settings_btn_, [](lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        if (app) {
            if (app->isSettingsVisible()) {
                app->hideSettings();
            } else {
                app->showSettings();
            }
        }
    }, LV_EVENT_CLICKED, this);
}

void ClockApp::createDateDisplay() {
    date_label_ = lv_label_create(container_);
    lv_label_set_text(date_label_, "Monday, 01 January 2024");
    lv_obj_set_style_text_color(date_label_, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(date_label_, LV_ALIGN_BOTTOM_MID, 0, -60);
}

void ClockApp::createSettingsPanel() {
    settings_panel_ = lv_obj_create(container_);
    lv_obj_set_size(settings_panel_, 420, 520);
    lv_obj_center(settings_panel_);
    lv_obj_set_style_bg_color(settings_panel_, lv_color_hex(0x16213E), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(settings_panel_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(settings_panel_, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_panel_, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(settings_panel_, lv_color_hex(0x0F3460), LV_PART_MAIN);
    lv_obj_set_style_pad_all(settings_panel_, 20, LV_PART_MAIN);
    lv_obj_add_flag(settings_panel_, LV_OBJ_FLAG_HIDDEN);

    // Title
    lv_obj_t* title = lv_label_create(settings_panel_);
    lv_label_set_text(title, "Clock Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(title, 320, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 0);

    // Background color section
    lv_obj_t* bg_label = lv_label_create(settings_panel_);
    lv_label_set_text(bg_label, "Background");
    lv_obj_set_style_text_color(bg_label, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_text_font(bg_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(bg_label, LV_ALIGN_TOP_LEFT, 0, 50);

    // R label
    lv_obj_t* r_label = lv_label_create(settings_panel_);
    lv_label_set_text(r_label, "R");
    lv_obj_set_style_text_color(r_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_text_font(r_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(r_label, LV_ALIGN_TOP_LEFT, 0, 80);

    // RGB sliders for background
    bg_color_slider_r_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(bg_color_slider_r_, 280, 20);
    lv_obj_align(bg_color_slider_r_, LV_ALIGN_TOP_LEFT, 30, 80);
    lv_obj_set_style_bg_color(bg_color_slider_r_, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    lv_slider_set_range(bg_color_slider_r_, 0, 255);
    lv_slider_set_value(bg_color_slider_r_, (bg_color_ >> 16) & 0xFF, LV_ANIM_OFF);

    // G label
    lv_obj_t* g_label = lv_label_create(settings_panel_);
    lv_label_set_text(g_label, "G");
    lv_obj_set_style_text_color(g_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(g_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(g_label, LV_ALIGN_TOP_LEFT, 0, 110);

    bg_color_slider_g_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(bg_color_slider_g_, 280, 20);
    lv_obj_align(bg_color_slider_g_, LV_ALIGN_TOP_LEFT, 30, 110);
    lv_obj_set_style_bg_color(bg_color_slider_g_, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_slider_set_range(bg_color_slider_g_, 0, 255);
    lv_slider_set_value(bg_color_slider_g_, (bg_color_ >> 8) & 0xFF, LV_ANIM_OFF);

    // B label
    lv_obj_t* b_label = lv_label_create(settings_panel_);
    lv_label_set_text(b_label, "B");
    lv_obj_set_style_text_color(b_label, lv_color_hex(0x0000FF), LV_PART_MAIN);
    lv_obj_set_style_text_font(b_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(b_label, LV_ALIGN_TOP_LEFT, 0, 140);

    bg_color_slider_b_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(bg_color_slider_b_, 280, 20);
    lv_obj_align(bg_color_slider_b_, LV_ALIGN_TOP_LEFT, 30, 140);
    lv_obj_set_style_bg_color(bg_color_slider_b_, lv_color_hex(0x0000FF), LV_PART_INDICATOR);
    lv_slider_set_range(bg_color_slider_b_, 0, 255);
    lv_slider_set_value(bg_color_slider_b_, bg_color_ & 0xFF, LV_ANIM_OFF);

    // Background color preview
    bg_color_preview_ = lv_obj_create(settings_panel_);
    lv_obj_set_size(bg_color_preview_, 60, 80);
    lv_obj_align(bg_color_preview_, LV_ALIGN_TOP_RIGHT, 0, 80);
    lv_obj_set_style_bg_color(bg_color_preview_, lv_color_hex(bg_color_), LV_PART_MAIN);
    lv_obj_set_style_radius(bg_color_preview_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(bg_color_preview_, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(bg_color_preview_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_clear_flag(bg_color_preview_, LV_OBJ_FLAG_SCROLLABLE);

    // Text color section
    lv_obj_t* text_label = lv_label_create(settings_panel_);
    lv_label_set_text(text_label, "Text Color");
    lv_obj_set_style_text_color(text_label, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_text_font(text_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(text_label, LV_ALIGN_TOP_LEFT, 0, 190);

    // R label for text
    lv_obj_t* tr_label = lv_label_create(settings_panel_);
    lv_label_set_text(tr_label, "R");
    lv_obj_set_style_text_color(tr_label, lv_color_hex(0xFF0000), LV_PART_MAIN);
    lv_obj_set_style_text_font(tr_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(tr_label, LV_ALIGN_TOP_LEFT, 0, 220);

    // RGB sliders for text
    text_color_slider_r_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(text_color_slider_r_, 280, 20);
    lv_obj_align(text_color_slider_r_, LV_ALIGN_TOP_LEFT, 30, 220);
    lv_obj_set_style_bg_color(text_color_slider_r_, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
    lv_slider_set_range(text_color_slider_r_, 0, 255);
    lv_slider_set_value(text_color_slider_r_, (text_color_ >> 16) & 0xFF, LV_ANIM_OFF);

    // G label for text
    lv_obj_t* tg_label = lv_label_create(settings_panel_);
    lv_label_set_text(tg_label, "G");
    lv_obj_set_style_text_color(tg_label, lv_color_hex(0x00FF00), LV_PART_MAIN);
    lv_obj_set_style_text_font(tg_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(tg_label, LV_ALIGN_TOP_LEFT, 0, 250);

    text_color_slider_g_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(text_color_slider_g_, 280, 20);
    lv_obj_align(text_color_slider_g_, LV_ALIGN_TOP_LEFT, 30, 250);
    lv_obj_set_style_bg_color(text_color_slider_g_, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
    lv_slider_set_range(text_color_slider_g_, 0, 255);
    lv_slider_set_value(text_color_slider_g_, (text_color_ >> 8) & 0xFF, LV_ANIM_OFF);

    // B label for text
    lv_obj_t* tb_label = lv_label_create(settings_panel_);
    lv_label_set_text(tb_label, "B");
    lv_obj_set_style_text_color(tb_label, lv_color_hex(0x0000FF), LV_PART_MAIN);
    lv_obj_set_style_text_font(tb_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(tb_label, LV_ALIGN_TOP_LEFT, 0, 280);

    text_color_slider_b_ = lv_slider_create(settings_panel_);
    lv_obj_set_size(text_color_slider_b_, 280, 20);
    lv_obj_align(text_color_slider_b_, LV_ALIGN_TOP_LEFT, 30, 280);
    lv_obj_set_style_bg_color(text_color_slider_b_, lv_color_hex(0x0000FF), LV_PART_INDICATOR);
    lv_slider_set_range(text_color_slider_b_, 0, 255);
    lv_slider_set_value(text_color_slider_b_, text_color_ & 0xFF, LV_ANIM_OFF);

    // Text color preview
    text_color_preview_ = lv_obj_create(settings_panel_);
    lv_obj_set_size(text_color_preview_, 60, 80);
    lv_obj_align(text_color_preview_, LV_ALIGN_TOP_RIGHT, 0, 220);
    lv_obj_set_style_bg_color(text_color_preview_, lv_color_hex(text_color_), LV_PART_MAIN);
    lv_obj_set_style_radius(text_color_preview_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(text_color_preview_, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(text_color_preview_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_clear_flag(text_color_preview_, LV_OBJ_FLAG_SCROLLABLE);

    // Close button
    lv_obj_t* close_btn = lv_btn_create(settings_panel_);
    lv_obj_set_size(close_btn, 120, 50);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xE94560), LV_PART_MAIN);
    lv_obj_set_style_radius(close_btn, 10, LV_PART_MAIN);

    lv_obj_t* close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "Close");
    lv_obj_set_style_text_color(close_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(close_label);

    lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        if (app) {
            app->hideSettings();
        }
    }, LV_EVENT_CLICKED, this);

    // Slider event callbacks for background color
    auto bg_slider_cb = [](lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        if (app) {
            int r = lv_slider_get_value(app->bg_color_slider_r_);
            int g = lv_slider_get_value(app->bg_color_slider_g_);
            int b = lv_slider_get_value(app->bg_color_slider_b_);
            uint32_t color = (r << 16) | (g << 8) | b;
            app->setBackgroundColor(color);
        }
    };

    lv_obj_add_event_cb(bg_color_slider_r_, bg_slider_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(bg_color_slider_g_, bg_slider_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(bg_color_slider_b_, bg_slider_cb, LV_EVENT_VALUE_CHANGED, this);

    // Slider event callbacks for text color
    auto text_slider_cb = [](lv_event_t* e) {
        ClockApp* app = static_cast<ClockApp*>(lv_event_get_user_data(e));
        if (app) {
            int r = lv_slider_get_value(app->text_color_slider_r_);
            int g = lv_slider_get_value(app->text_color_slider_g_);
            int b = lv_slider_get_value(app->text_color_slider_b_);
            uint32_t color = (r << 16) | (g << 8) | b;
            app->setTextColor(color);
        }
    };

    lv_obj_add_event_cb(text_color_slider_r_, text_slider_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(text_color_slider_g_, text_slider_cb, LV_EVENT_VALUE_CHANGED, this);
    lv_obj_add_event_cb(text_color_slider_b_, text_slider_cb, LV_EVENT_VALUE_CHANGED, this);
}

void ClockApp::update(uint32_t now_ms) {
    if (!container_) return;

    // Update every second
    if (now_ms - last_update_ms_ < 1000) return;
    last_update_ms_ = now_ms;

    updateTime();
    updateDate();
}

void ClockApp::updateTime() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    int second = timeinfo->tm_sec;

    // Only update if changed
    if (second == last_second_) {
        return;
    }
    last_second_ = second;

    // Update time label (HH:MM:SS)
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", 
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    lv_label_set_text(time_label_, buf);
}

void ClockApp::updateDate() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);

    const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", 
                          "Thursday", "Friday", "Saturday"};
    const char* months[] = {"January", "February", "March", "April",
                            "May", "June", "July", "August",
                            "September", "October", "November", "December"};

    char buf[64];
    snprintf(buf, sizeof(buf), "%s, %02d %s %d",
             days[timeinfo->tm_wday],
             timeinfo->tm_mday,
             months[timeinfo->tm_mon],
             timeinfo->tm_year + 1900);
    lv_label_set_text(date_label_, buf);
}

void ClockApp::setTextColor(uint32_t color_hex) {
    text_color_ = color_hex;
    
    if (time_label_) {
        lv_obj_set_style_text_color(time_label_, lv_color_hex(text_color_), LV_PART_MAIN);
    }
    if (date_label_) {
        lv_obj_set_style_text_color(date_label_, lv_color_hex(text_color_), LV_PART_MAIN);
    }
    if (text_color_preview_) {
        lv_obj_set_style_bg_color(text_color_preview_, lv_color_hex(text_color_), LV_PART_MAIN);
    }
}

void ClockApp::setBackgroundColor(uint32_t color_hex) {
    bg_color_ = color_hex;
    
    if (container_) {
        lv_obj_set_style_bg_color(container_, lv_color_hex(bg_color_), LV_PART_MAIN);
    }
    if (bg_color_preview_) {
        lv_obj_set_style_bg_color(bg_color_preview_, lv_color_hex(bg_color_), LV_PART_MAIN);
    }
}

void ClockApp::showSettings() {
    if (settings_panel_) {
        lv_obj_clear_flag(settings_panel_, LV_OBJ_FLAG_HIDDEN);
        settings_visible_ = true;
    }
}

void ClockApp::hideSettings() {
    if (settings_panel_) {
        lv_obj_add_flag(settings_panel_, LV_OBJ_FLAG_HIDDEN);
        settings_visible_ = false;
    }
}

bool ClockApp::isSettingsVisible() const {
    return settings_visible_;
}

} // namespace ui
