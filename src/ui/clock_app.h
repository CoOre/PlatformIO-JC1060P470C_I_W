#pragma once

#include "lvgl.h"
#include <cstdint>

// Forward declaration for settings
namespace settings {
    class SettingsStore;
}

namespace ui {

/**
 * @brief Digital Clock application - large digital clock with date
 * 
 * Features:
 * - Large digital time display (HH:MM:SS)
 * - Date display
 * - Colors follow system theme
 */
class ClockApp {
public:
    ClockApp();
    ~ClockApp();

    /**
     * @brief Create the app on given parent
     * @param parent LVGL parent object
     * @return true if successful
     */
    bool create(lv_obj_t* parent);

    /**
     * @brief Destroy the app and free resources
     */
    void destroy();

    /**
     * @brief Update app logic - updates time display
     * @param now_ms current time in milliseconds
     */
    void update(uint32_t now_ms);

    /**
     * @brief Check if app is currently created
     */
    bool isActive() const { return container_ != nullptr; }

private:
    void setupContainer(lv_obj_t* parent);
    void createTimeDisplay();
    void createDateDisplay();
    void applyThemeColors();
    void updateTime();
    void updateDate();

    // UI Elements
    lv_obj_t* container_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* date_label_ = nullptr;

    // State
    uint32_t last_update_ms_ = 0;
    int last_second_ = -1;
};

} // namespace ui
