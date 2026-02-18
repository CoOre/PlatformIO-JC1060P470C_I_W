#include "screen_time.h"
#include "../i18n/i18n.h"
#include "../services/time_service.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "TimeScreen";

// Common timezones (POSIX format)
struct TimezoneOption {
    const char* name;
    const char* tz;
};

static const TimezoneOption TIMEZONES[] = {
    { "UTC", "UTC0" },
    { "Europe/Moscow", "MSK-3" },
    { "Europe/London", "GMT0BST,M3.5.0/1,M10.5.0" },
    { "Europe/Paris", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "Europe/Berlin", "CET-1CEST,M3.5.0,M10.5.0/3" },
    { "US/Eastern", "EST5EDT,M3.2.0,M11.1.0" },
    { "US/Pacific", "PST8PDT,M3.2.0,M11.1.0" },
    { "Asia/Tokyo", "JST-9" },
    { "Asia/Shanghai", "CST-8" },
};

static const size_t TIMEZONE_COUNT = sizeof(TIMEZONES) / sizeof(TIMEZONES[0]);

// ============================================================================
// Lifecycle
// ============================================================================

bool TimeScreen::create(lv_obj_t* parent) {
    if (!parent) return false;
    
    setupContainer(parent);
    theme_ = currentThemeColors();
    lv_obj_set_style_bg_color(container_, theme_.screen_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(container_, 12, LV_PART_MAIN);
    lv_obj_set_scroll_dir(container_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_AUTO);
    
    createHeader();
    createCurrentTimeSection();
    createTimezoneSection();
    createNtpSection();
    createManualSetSection();
    
    // Register for time events
    TimeService::instance().setUIEventObject(container_);
    lv_obj_add_event_cb(container_, onEventReceived, LV_EVENT_VALUE_CHANGED, this);
    
    ESP_LOGI(TAG, "Time screen created");
    return true;
}

void TimeScreen::destroy() {
    TimeService::instance().setUIEventObject(nullptr);
    
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    
    ESP_LOGI(TAG, "Time screen destroyed");
}

void TimeScreen::update(uint32_t now_ms) {
    // Update time display every second
    static uint32_t last_update = 0;
    if (now_ms - last_update >= 1000) {
        last_update = now_ms;
        refreshTimeDisplay();
    }
}

// ============================================================================
// UI Creation
// ============================================================================

void TimeScreen::createHeader() {
    header_ = lv_obj_create(container_);
    lv_obj_set_size(header_, LV_PCT(100), 56);
    lv_obj_set_style_bg_color(header_, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(header_, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(header_, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(header_, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(header_, 0, LV_PART_MAIN);
    lv_obj_set_layout(header_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(TIME_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, theme_.title, LV_PART_MAIN);
}

void TimeScreen::createCurrentTimeSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(section, 6, LV_PART_MAIN);
    
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(TIME_CURRENT));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    time_label_ = lv_label_create(section);
    lv_obj_set_style_text_font(time_label_, &lv_font_roboto_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label_, theme_.title, LV_PART_MAIN);
    
    date_label_ = lv_label_create(section);
    lv_obj_set_style_text_color(date_label_, theme_.muted, LV_PART_MAIN);
    
    refreshTimeDisplay();
}

void TimeScreen::createTimezoneSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(section, 10, LV_PART_MAIN);
    
    lv_obj_t* row = lv_obj_create(section);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, S(TIME_TIMEZONE));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    timezone_dropdown_ = lv_dropdown_create(row);
    lv_obj_set_size(timezone_dropdown_, 280, 40);
    
    // Build options
    char options[512] = {};
    for (size_t i = 0; i < TIMEZONE_COUNT; i++) {
        if (i > 0) strcat(options, "\n");
        strcat(options, TIMEZONES[i].name);
    }
    lv_dropdown_set_options(timezone_dropdown_, options);
    
    // Set current
    const char* current_tz = TimeService::instance().getTimezone();
    for (size_t i = 0; i < TIMEZONE_COUNT; i++) {
        if (strcmp(TIMEZONES[i].tz, current_tz) == 0) {
            lv_dropdown_set_selected(timezone_dropdown_, i);
            break;
        }
    }
    
    lv_obj_add_event_cb(timezone_dropdown_, onTimezoneChanged, LV_EVENT_VALUE_CHANGED, this);
}

void TimeScreen::createNtpSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(section, 10, LV_PART_MAIN);
    
    lv_obj_t* top_row = lv_obj_create(section);
    lv_obj_set_size(top_row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_row, 0, LV_PART_MAIN);
    lv_obj_set_layout(top_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* switch_label = lv_label_create(top_row);
    lv_label_set_text(switch_label, S(TIME_NTP_ENABLED));
    lv_obj_set_style_text_font(switch_label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(switch_label, theme_.title, LV_PART_MAIN);
    
    ntp_switch_ = lv_switch_create(top_row);
    
    if (TimeService::instance().isNtpEnabled()) {
        lv_obj_add_state(ntp_switch_, LV_STATE_CHECKED);
    }
    
    lv_obj_add_event_cb(ntp_switch_, onNtpToggle, LV_EVENT_VALUE_CHANGED, this);
    
    lv_obj_t* server_label = lv_label_create(section);
    lv_label_set_text(server_label, S(TIME_NTP_SERVER));
    lv_obj_set_style_text_font(server_label, &lv_font_roboto_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(server_label, theme_.muted, LV_PART_MAIN);
    
    ntp_server_ta_ = lv_textarea_create(section);
    lv_obj_set_size(ntp_server_ta_, LV_PCT(100), 40);
    lv_textarea_set_one_line(ntp_server_ta_, true);
    lv_textarea_set_text(ntp_server_ta_, TimeService::instance().getNtpServer());
    lv_obj_set_style_border_color(ntp_server_ta_, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_border_width(ntp_server_ta_, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(ntp_server_ta_, 10, LV_PART_MAIN);
    
    lv_obj_t* sync_btn = lv_btn_create(section);
    lv_obj_set_size(sync_btn, 160, 40);
    lv_obj_set_style_bg_color(sync_btn, theme_.accent, LV_PART_MAIN);
    lv_obj_set_style_radius(sync_btn, 10, LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(sync_btn);
    lv_label_set_text(btn_label, S(TIME_SYNC_NOW));
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(sync_btn, onSyncNowClicked, LV_EVENT_CLICKED, this);
    
    ntp_status_label_ = lv_label_create(section);
    lv_obj_set_style_text_color(ntp_status_label_, theme_.muted, LV_PART_MAIN);
    refreshNtpStatus();
}

void TimeScreen::createManualSetSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    
    lv_obj_t* manual_btn = lv_btn_create(section);
    lv_obj_set_size(manual_btn, LV_PCT(100), 46);
    lv_obj_set_style_bg_color(manual_btn, theme_.accent, LV_PART_MAIN);
    lv_obj_set_style_radius(manual_btn, 10, LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(manual_btn);
    lv_label_set_text(btn_label, S(TIME_MANUAL_SET));
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(manual_btn, onManualSetClicked, LV_EVENT_CLICKED, this);
}

// ============================================================================
// UI Updates
// ============================================================================

void TimeScreen::refreshTimeDisplay() {
    TimeService& time_svc = TimeService::instance();
    
    // Time
    lv_label_set_text(time_label_, time_svc.getTimeString());
    
    // Date
    lv_label_set_text(date_label_, time_svc.getDateString());
}

void TimeScreen::refreshNtpStatus() {
    TimeService& time_svc = TimeService::instance();
    
    if (!time_svc.isNtpEnabled()) {
        lv_label_set_text(ntp_status_label_, "NTP: Off");
    } else if (time_svc.isSynced()) {
        lv_label_set_text(ntp_status_label_, "NTP: Synced");
    } else {
        lv_label_set_text(ntp_status_label_, "NTP: Waiting...");
    }
}

// ============================================================================
// Event Handlers
// ============================================================================

void TimeScreen::onTimezoneChanged(lv_event_t* e) {
    lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    if (selected < TIMEZONE_COUNT) {
        TimeService::instance().setTimezone(TIMEZONES[selected].tz);
    }
}

void TimeScreen::onNtpToggle(lv_event_t* e) {
    lv_obj_t* sw = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    TimeService::instance().setNtpEnabled(enabled);
}

void TimeScreen::onSyncNowClicked(lv_event_t* e) {
    TimeService::instance().syncNtp();
}

void TimeScreen::onManualSetClicked(lv_event_t* e) {
    // For simplicity, set current time + 1 minute
    // In a full implementation, show a dialog with rollers
    TimeScreen* screen = static_cast<TimeScreen*>(lv_event_get_user_data(e));
    
    // Show a simple dialog (placeholder)
    // In production, create rollers for year/month/day/hour/minute
    time_t now = TimeService::instance().getTimestamp();
    TimeService::instance().setTimestamp(now);  // Just refresh
}

void TimeScreen::onEventReceived(lv_event_t* e) {
    TimeScreen* screen = static_cast<TimeScreen*>(lv_event_get_user_data(e));
    TimeEventData* evt = static_cast<TimeEventData*>(lv_event_get_param(e));
    
    if (evt) {
        screen->handleTimeEvent(*evt);
        delete evt;
    }
}

void TimeScreen::handleTimeEvent(const TimeEventData& evt) {
    switch (evt.event) {
        case TimeEvent::TIME_UPDATED:
            refreshTimeDisplay();
            break;
        case TimeEvent::TIME_SYNCED:
        case TimeEvent::NTP_ENABLED:
        case TimeEvent::NTP_DISABLED:
            refreshNtpStatus();
            break;
        default:
            break;
    }
}

bool TimeScreen::onBack() {
    // Return false to let router handle back navigation
    return false;
}

} // namespace settings
