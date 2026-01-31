#include "ui_status_bar.h"
#include "ui_manager.h"

namespace ui {

StatusBar::StatusBar() = default;

StatusBar::~StatusBar() {
    // LVGL objects are automatically cleaned up with their parent
}

bool StatusBar::create(lv_obj_t* parent) {
    if (!parent) return false;
    
    // Create main container
    container_ = lv_obj_create(parent);
    lv_obj_set_width(container_, LV_PCT(100));
    lv_obj_set_height(container_, height());
    
    // Style the container
    lv_obj_set_style_bg_color(container_, lv_color_hex(0x0C101B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(container_, lv_color_hex(0x141C2B), LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(container_, LV_GRAD_DIR_VER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(container_, 18, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(container_, 8, LV_PART_MAIN);
    lv_obj_set_style_margin_all(container_, 0, LV_PART_MAIN);
    
    // Layout
    lv_obj_set_layout(container_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container_, 12, LV_PART_MAIN);
    
    createBackButton();
    createTimeLabel();
    createStatusIcons();
    
    return true;
}

void StatusBar::createBackButton() {
    back_button_ = lv_btn_create(container_);
    lv_obj_set_size(back_button_, 60, 22);
    lv_obj_set_style_bg_opa(back_button_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_button_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_button_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(back_button_, 0, LV_PART_MAIN);
    lv_obj_add_flag(back_button_, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label = lv_label_create(back_button_);
    lv_label_set_text(label, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_color(label, lv_color_hex(0xE6EDF7), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label);

    lv_obj_add_event_cb(back_button_, backButtonClickHandler, LV_EVENT_CLICKED, this);
}

void StatusBar::setBackButtonVisible(bool visible) {
    if (back_button_) {
        if (visible) {
            lv_obj_clear_flag(back_button_, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(back_button_, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void StatusBar::setBackButtonCallback(std::function<void()> callback) {
    back_callback_ = callback;
}

void StatusBar::backButtonClickHandler(lv_event_t* e) {
    auto* status_bar = static_cast<StatusBar*>(lv_event_get_user_data(e));
    if (status_bar && status_bar->back_callback_) {
        status_bar->back_callback_();
    }
}

void StatusBar::createTimeLabel() {
    time_label_ = lv_label_create(container_);
    
    // Style
    lv_obj_set_style_text_color(time_label_, lv_color_hex(0xE6EDF7), LV_PART_MAIN);
    lv_obj_set_style_text_font(time_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(time_label_, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(time_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_flex_grow(time_label_, 1);
    
    // Initial text
    lv_label_set_text(time_label_, "00:00:00");
}

void StatusBar::createStatusIcons() {
    // Right side container for icons
    status_right_box_ = lv_obj_create(container_);
    lv_obj_set_size(status_right_box_, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(status_right_box_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status_right_box_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status_right_box_, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Transparent background
    lv_obj_set_style_bg_opa(status_right_box_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(status_right_box_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(status_right_box_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_gap(status_right_box_, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(status_right_box_, 0, LV_PART_MAIN);
    lv_obj_set_style_min_width(status_right_box_, 100, LV_PART_MAIN);
    
    // WiFi label
    wifi_label_ = lv_label_create(status_right_box_);
    lv_obj_set_style_text_color(wifi_label_, lv_color_hex(0xE6EDF7), LV_PART_MAIN);
    lv_obj_set_style_text_font(wifi_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(wifi_label_, LV_SYMBOL_WIFI);
    
    // Battery label
    battery_label_ = lv_label_create(status_right_box_);
    lv_obj_set_style_text_color(battery_label_, lv_color_hex(0xE6EDF7), LV_PART_MAIN);
    lv_obj_set_style_text_font(battery_label_, &lv_font_montserrat_14, LV_PART_MAIN);
    setBatteryLevel(100, false);
}

void StatusBar::updateTime(uint32_t now_ms) {
    if (!time_label_) return;
    
    uint32_t total_seconds = now_ms / 1000;
    uint32_t hours = (total_seconds / 3600U) % 24U;
    uint32_t minutes = (total_seconds / 60U) % 60U;
    uint32_t seconds = total_seconds % 60U;
    
    lv_label_set_text_fmt(time_label_, "%02u:%02u:%02u",
                          static_cast<unsigned>(hours),
                          static_cast<unsigned>(minutes),
                          static_cast<unsigned>(seconds));
}

void StatusBar::setWiFiConnected(bool connected) {
    wifi_connected_ = connected;
    if (wifi_label_) {
        lv_label_set_text(wifi_label_, connected ? LV_SYMBOL_WIFI : "");
    }
}

void StatusBar::setBatteryLevel(uint8_t level, bool charging) {
    battery_level_ = level;
    charging_ = charging;
    
    if (battery_label_) {
        const char* icon = getBatteryIcon(level, charging);
        lv_label_set_text_fmt(battery_label_, "%s %u%%", icon, static_cast<unsigned>(level));
    }
}

const char* StatusBar::getBatteryIcon(uint8_t level, bool charging) const {
    if (charging) return LV_SYMBOL_CHARGE;
    if (level > 85) return LV_SYMBOL_BATTERY_FULL;
    if (level > 65) return LV_SYMBOL_BATTERY_3;
    if (level > 45) return LV_SYMBOL_BATTERY_2;
    if (level > 25) return LV_SYMBOL_BATTERY_1;
    return LV_SYMBOL_BATTERY_EMPTY;
}

void StatusBar::refresh(uint32_t now_ms) {
    updateTime(now_ms);
    setWiFiConnected(wifi_connected_);
    setBatteryLevel(battery_level_, charging_);
}

} // namespace ui
