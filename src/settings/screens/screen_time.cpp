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
    lv_obj_set_size(header_, LV_PCT(100), 50);
    lv_obj_set_style_pad_all(header_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 0, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(TIME_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_center(title);
}

void TimeScreen::createCurrentTimeSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Current time label
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(TIME_CURRENT));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Time
    time_label_ = lv_label_create(section);
    lv_obj_set_style_text_font(time_label_, &lv_font_roboto_24, LV_PART_MAIN);
    lv_obj_align(time_label_, LV_ALIGN_TOP_LEFT, 0, 30);
    
    // Date
    date_label_ = lv_label_create(section);
    lv_obj_align(date_label_, LV_ALIGN_TOP_LEFT, 0, 60);
    
    refreshTimeDisplay();
}

void TimeScreen::createTimezoneSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Label
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(TIME_TIMEZONE));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Dropdown
    timezone_dropdown_ = lv_dropdown_create(section);
    lv_obj_set_size(timezone_dropdown_, 280, 40);
    lv_obj_align(timezone_dropdown_, LV_ALIGN_TOP_LEFT, 0, 30);
    
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
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // NTP enable switch
    lv_obj_t* switch_label = lv_label_create(section);
    lv_label_set_text(switch_label, S(TIME_NTP_ENABLED));
    lv_obj_align(switch_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    ntp_switch_ = lv_switch_create(section);
    lv_obj_align(ntp_switch_, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    if (TimeService::instance().isNtpEnabled()) {
        lv_obj_add_state(ntp_switch_, LV_STATE_CHECKED);
    }
    
    lv_obj_add_event_cb(ntp_switch_, onNtpToggle, LV_EVENT_VALUE_CHANGED, this);
    
    // NTP Server
    lv_obj_t* server_label = lv_label_create(section);
    lv_label_set_text(server_label, S(TIME_NTP_SERVER));
    lv_obj_align(server_label, LV_ALIGN_TOP_LEFT, 0, 40);
    
    ntp_server_ta_ = lv_textarea_create(section);
    lv_obj_set_size(ntp_server_ta_, 200, 40);
    lv_obj_align(ntp_server_ta_, LV_ALIGN_TOP_LEFT, 0, 65);
    lv_textarea_set_one_line(ntp_server_ta_, true);
    lv_textarea_set_text(ntp_server_ta_, TimeService::instance().getNtpServer());
    
    // Sync now button
    lv_obj_t* sync_btn = lv_btn_create(section);
    lv_obj_set_size(sync_btn, 120, 40);
    lv_obj_align(sync_btn, LV_ALIGN_TOP_RIGHT, 0, 65);
    lv_obj_set_style_bg_color(sync_btn, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(sync_btn);
    lv_label_set_text(btn_label, S(TIME_SYNC_NOW));
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(sync_btn, onSyncNowClicked, LV_EVENT_CLICKED, this);
    
    // Status
    ntp_status_label_ = lv_label_create(section);
    lv_obj_align(ntp_status_label_, LV_ALIGN_TOP_LEFT, 0, 115);
    refreshNtpStatus();
}

void TimeScreen::createManualSetSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Manual set button
    lv_obj_t* manual_btn = lv_btn_create(section);
    lv_obj_set_size(manual_btn, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(manual_btn, lv_color_hex(0xFF9800), LV_PART_MAIN);
    
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
