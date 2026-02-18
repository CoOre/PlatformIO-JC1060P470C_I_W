#include "clock_app.h"
#include "ui_manager.h"
#include "../settings/core/settings_store.h"
#include "../settings/core/theme.h"
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

    return true;
}

void ClockApp::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        time_label_ = nullptr;
        date_label_ = nullptr;
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

    // Apply theme colors
    applyThemeColors();
    
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);
}

void ClockApp::applyThemeColors() {
    if (!container_) return;
    
    settings::ThemeColors theme = settings::currentThemeColors();
    
    // Background follows screen background from theme
    lv_obj_set_style_bg_color(container_, theme.screen_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
}

void ClockApp::createTimeDisplay() {
    // Main time label (HH:MM:SS)
    time_label_ = lv_label_create(container_);
    lv_label_set_text(time_label_, "00:00:00");
    
    // Apply theme text color (title color for main time)
    settings::ThemeColors theme = settings::currentThemeColors();
    lv_obj_set_style_text_color(time_label_, theme.title, LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_48, LV_PART_MAIN);
    
    // Position in center
    lv_obj_center(time_label_);
}

void ClockApp::createDateDisplay() {
    date_label_ = lv_label_create(container_);
    lv_label_set_text(date_label_, "Monday, 01 January 2024");
    
    // Apply theme muted color for date
    settings::ThemeColors theme = settings::currentThemeColors();
    lv_obj_set_style_text_color(date_label_, theme.muted, LV_PART_MAIN);
    lv_obj_set_style_text_font(date_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(date_label_, LV_ALIGN_BOTTOM_MID, 0, -60);
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

} // namespace ui
