#pragma once

#include "lvgl.h"
#include <cstdint>

namespace ui {

/**
 * @brief Digital Clock application - large digital clock with date
 * 
 * Features:
 * - Large digital time display (HH:MM:SS)
 * - Date display
 * - Settings for text color and background color
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

    /**
     * @brief Set text color (for time/date labels)
     * @param color_hex RGB color in hex format (e.g., 0xFFFFFF)
     */
    void setTextColor(uint32_t color_hex);

    /**
     * @brief Set background color
     * @param color_hex RGB color in hex format (e.g., 0x000000)
     */
    void setBackgroundColor(uint32_t color_hex);

    /**
     * @brief Show settings panel
     */
    void showSettings();

    /**
     * @brief Hide settings panel
     */
    void hideSettings();

    /**
     * @brief Check if settings panel is visible
     */
    bool isSettingsVisible() const;

private:
    void setupContainer(lv_obj_t* parent);
    void createTimeDisplay();
    void createDateDisplay();
    void createSettingsPanel();
    void updateTime();
    void updateDate();

    // UI Elements
    lv_obj_t* container_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* date_label_ = nullptr;
    lv_obj_t* settings_panel_ = nullptr;
    lv_obj_t* settings_btn_ = nullptr;

    // Settings UI
    lv_obj_t* bg_color_slider_r_ = nullptr;
    lv_obj_t* bg_color_slider_g_ = nullptr;
    lv_obj_t* bg_color_slider_b_ = nullptr;
    lv_obj_t* text_color_slider_r_ = nullptr;
    lv_obj_t* text_color_slider_g_ = nullptr;
    lv_obj_t* text_color_slider_b_ = nullptr;
    lv_obj_t* bg_color_preview_ = nullptr;
    lv_obj_t* text_color_preview_ = nullptr;

    // Colors
    uint32_t text_color_ = 0xFFFFFF;
    uint32_t bg_color_ = 0x1A1A2E;

    // State
    uint32_t last_update_ms_ = 0;
    bool settings_visible_ = false;
    int last_second_ = -1;
};

} // namespace ui
