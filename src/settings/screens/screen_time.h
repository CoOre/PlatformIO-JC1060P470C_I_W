#pragma once

#include "../navigation/ui_router.h"
#include "../services/time_service.h"

namespace settings {

/**
 * @brief Time settings screen
 * 
 * Features:
 * - Current time/date display
 * - Timezone selection
 * - NTP enable/disable
 * - NTP server configuration
 * - Manual time setting
 */
class TimeScreen : public Screen {
public:
    TimeScreen() = default;
    ~TimeScreen() override = default;
    
    bool create(lv_obj_t* parent) override;
    void destroy() override;
    void update(uint32_t now_ms) override;
    ScreenType getType() const override { return ScreenType::TIME; }
    bool onBack() override;

private:
    void createHeader();
    void createCurrentTimeSection();
    void createTimezoneSection();
    void createNtpSection();
    void createManualSetSection();
    
    void refreshTimeDisplay();
    void refreshNtpStatus();
    
    static void onTimezoneChanged(lv_event_t* e);
    static void onNtpToggle(lv_event_t* e);
    static void onSyncNowClicked(lv_event_t* e);
    static void onManualSetClicked(lv_event_t* e);
    static void onEventReceived(lv_event_t* e);
    static void onTimeSetConfirm(lv_event_t* e);
    static void onTimeSetCancel(lv_event_t* e);
    
    void handleTimeEvent(const TimeEventData& evt);
    
    lv_obj_t* header_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* date_label_ = nullptr;
    lv_obj_t* timezone_dropdown_ = nullptr;
    lv_obj_t* ntp_switch_ = nullptr;
    lv_obj_t* ntp_server_ta_ = nullptr;
    lv_obj_t* ntp_status_label_ = nullptr;
    lv_obj_t* sync_now_btn_ = nullptr;
    
    // Manual set dialog
    lv_obj_t* manual_dialog_ = nullptr;
    lv_obj_t* year_roller_ = nullptr;
    lv_obj_t* month_roller_ = nullptr;
    lv_obj_t* day_roller_ = nullptr;
    lv_obj_t* hour_roller_ = nullptr;
    lv_obj_t* minute_roller_ = nullptr;
    lv_obj_t* set_btn_ = nullptr;
    lv_obj_t* cancel_btn_ = nullptr;
    
    bool showing_manual_dialog_ = false;
};

} // namespace settings
