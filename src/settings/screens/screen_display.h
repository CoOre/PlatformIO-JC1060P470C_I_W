#pragma once

#include "../navigation/ui_router.h"
#include "../core/theme.h"

namespace settings {

/**
 * @brief Display settings screen
 * 
 * Features:
 * - Brightness slider (0-100%)
 * - Backlight timeout selector (15s/30s/1m/5m/never)
 */
class DisplayScreen : public Screen {
public:
    DisplayScreen() = default;
    ~DisplayScreen() override = default;
    
    bool create(lv_obj_t* parent) override;
    void destroy() override;
    ScreenType getType() const override { return ScreenType::DISPLAY; }

private:
    void createHeader();
    void createBrightnessSection();
    void createIdleBrightnessSection();
    void createTimeoutSection();
    void createRotationSection();
    void createThemeSection();
    
    static void onBrightnessChanged(lv_event_t* e);
    static void onIdleBrightnessChanged(lv_event_t* e);
    static void onTimeoutSelected(lv_event_t* e);
    static void onRotationSelected(lv_event_t* e);
    static void onThemeToggled(lv_event_t* e);
    
    lv_obj_t* header_ = nullptr;
    lv_obj_t* brightness_slider_ = nullptr;
    lv_obj_t* brightness_label_ = nullptr;
    lv_obj_t* idle_brightness_slider_ = nullptr;
    lv_obj_t* idle_brightness_label_ = nullptr;
    lv_obj_t* timeout_dropdown_ = nullptr;
    lv_obj_t* rotation_dropdown_ = nullptr;
    lv_obj_t* theme_switch_ = nullptr;
    lv_obj_t* theme_value_label_ = nullptr;
    ThemeColors theme_;
};

} // namespace settings
