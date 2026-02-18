#pragma once

#include "../navigation/ui_router.h"

namespace settings {

/**
 * @brief System settings screen
 * 
 * Features:
 * - Firmware version
 * - Uptime
 * - Free heap/PSRAM
 * - Reset settings button
 * - Reboot button
 */
class SystemScreen : public Screen {
public:
    SystemScreen() = default;
    ~SystemScreen() override = default;
    
    bool create(lv_obj_t* parent) override;
    void destroy() override;
    void update(uint32_t now_ms) override;
    ScreenType getType() const override { return ScreenType::SYSTEM; }

private:
    void createHeader();
    void createInfoSection();
    void createActionsSection();
    void createConfirmDialog();
    
    void refreshInfo();
    void showConfirmDialog(const char* message, void (*callback)());
    void hideConfirmDialog();
    
    static void onResetClicked(lv_event_t* e);
    static void onRebootClicked(lv_event_t* e);
    static void onConfirmYes(lv_event_t* e);
    static void onConfirmNo(lv_event_t* e);
    
    lv_obj_t* header_ = nullptr;
    lv_obj_t* version_label_ = nullptr;
    lv_obj_t* uptime_label_ = nullptr;
    lv_obj_t* heap_label_ = nullptr;
    lv_obj_t* psram_label_ = nullptr;
    
    lv_obj_t* confirm_dialog_ = nullptr;
    lv_obj_t* confirm_label_ = nullptr;
    void (*confirm_callback_)() = nullptr;
    
    bool showing_confirm_ = false;
};

} // namespace settings
