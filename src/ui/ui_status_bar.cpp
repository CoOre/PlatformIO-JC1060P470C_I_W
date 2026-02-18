#include "ui_status_bar.h"
#include "ui_manager.h"
#include "../settings/services/time_service.h"

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
    
    // Disable scrolling completely
    lv_obj_set_scroll_dir(container_, LV_DIR_NONE);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_OFF);
    
    createBackButton();
    createTimeLabel();
    createStatusIcons();

    applyTheme(settings::currentThemeColors());
    
    return true;
}

void StatusBar::createBackButton() {
    // Create a placeholder container that always takes up space
    lv_obj_t* placeholder = lv_obj_create(container_);
    lv_obj_set_size(placeholder, 60, 1);
    lv_obj_set_style_bg_opa(placeholder, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(placeholder, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(placeholder, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(placeholder, 0, LV_PART_MAIN);
    
    back_button_ = lv_btn_create(container_);
    lv_obj_set_size(back_button_, 60, 22);
    lv_obj_set_style_bg_opa(back_button_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(back_button_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_button_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(back_button_, 0, LV_PART_MAIN);
    lv_obj_add_flag(back_button_, LV_OBJ_FLAG_HIDDEN);
    
    // Float the button over the placeholder
    lv_obj_add_flag(back_button_, LV_OBJ_FLAG_FLOATING);
    lv_obj_align(back_button_, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t* label = lv_label_create(back_button_);
    lv_label_set_text(label, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(label, &lv_font_roboto_14, LV_PART_MAIN);
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
    // Create time label with absolute positioning - centered on screen
    time_label_ = lv_label_create(container_);
    
    // Remove from flex layout - use absolute positioning
    lv_obj_remove_flag(time_label_, LV_OBJ_FLAG_FLEX_IN_NEW_TRACK);
    lv_obj_set_pos(time_label_, 0, 0);
    
    // Center horizontally on the screen (1024px width / 2 = 512)
    // Account for label width (80px) so center of text is at screen center
    lv_obj_align(time_label_, LV_ALIGN_TOP_MID, 0, 6);  // 6px from top of status bar
    
    // Style
    lv_obj_set_style_text_font(time_label_, &lv_font_roboto_14, LV_PART_MAIN);
    lv_obj_set_style_text_letter_space(time_label_, 2, LV_PART_MAIN);
    lv_obj_set_style_text_align(time_label_, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    
    // Fixed width to keep time centered regardless of content changes
    lv_obj_set_width(time_label_, 80);
    
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
    lv_obj_set_style_text_font(wifi_label_, &lv_font_roboto_14, LV_PART_MAIN);
    lv_label_set_text(wifi_label_, LV_SYMBOL_WIFI);
    
    // Battery label
    battery_label_ = lv_label_create(status_right_box_);
    lv_obj_set_style_text_font(battery_label_, &lv_font_roboto_14, LV_PART_MAIN);
    setBatteryLevel(100, false);
}

void StatusBar::updateTime(uint32_t now_ms) {
    if (!time_label_) return;
    
    // Get real time from TimeService
    struct tm timeinfo = settings::TimeService::instance().getLocalTime();
    
    lv_label_set_text_fmt(time_label_, "%02d:%02d:%02d",
                          timeinfo.tm_hour,
                          timeinfo.tm_min,
                          timeinfo.tm_sec);
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

void StatusBar::applyTheme(const settings::ThemeColors& theme) {
    if (!container_) return;

    lv_obj_set_style_bg_color(container_, theme.status_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_dir(container_, LV_GRAD_DIR_NONE, LV_PART_MAIN);
    lv_obj_set_style_bg_grad_color(container_, theme.status_bg, LV_PART_MAIN);

    if (time_label_) {
        lv_obj_set_style_text_color(time_label_, theme.status_fg, LV_PART_MAIN);
    }
    if (wifi_label_) {
        lv_obj_set_style_text_color(wifi_label_, theme.status_fg, LV_PART_MAIN);
    }
    if (battery_label_) {
        lv_obj_set_style_text_color(battery_label_, theme.status_fg, LV_PART_MAIN);
    }
    if (back_button_) {
        lv_obj_t* label = lv_obj_get_child(back_button_, 0);
        if (label) {
            lv_obj_set_style_text_color(label, theme.status_fg, LV_PART_MAIN);
        }
    }
}

} // namespace ui
