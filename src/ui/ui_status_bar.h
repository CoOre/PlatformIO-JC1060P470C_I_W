#pragma once

#include "lvgl.h"
#include <cstdint>
#include <functional>
#include "settings/core/theme.h"

namespace ui {

/**
 * @brief Status bar component
 * 
 * Displays time, WiFi status, and battery level at the top of the screen.
 */
class StatusBar {
public:
    StatusBar();
    ~StatusBar();
    
    /**
     * @brief Create the status bar on the given parent
     * @param parent LVGL parent object
     * @return true if successful
     */
    bool create(lv_obj_t* parent);
    
    /**
     * @brief Update the displayed time
     * @param now_ms current time in milliseconds
     */
    void updateTime(uint32_t now_ms);
    
    /**
     * @brief Update WiFi status display
     * @param connected true if WiFi is connected
     */
    void setWiFiConnected(bool connected);
    
    /**
     * @brief Update battery display
     * @param level battery level 0-100
     * @param charging true if charging
     */
    void setBatteryLevel(uint8_t level, bool charging = false);
    
    /**
     * @brief Refresh all status elements
     */
    void refresh(uint32_t now_ms);

    /**
     * @brief Set back button visibility
     */
    void setBackButtonVisible(bool visible);

    /**
     * @brief Set callback for back button click
     */
    void setBackButtonCallback(std::function<void()> callback);

    /**
     * @brief Apply current theme to status bar
     */
    void applyTheme(const settings::ThemeColors& theme);

    /**
     * @brief Get the height of the status bar
     */
    static constexpr lv_coord_t height() { return 28; }
    
private:
    void setupStyles();
    void createTimeLabel();
    void createStatusIcons();
    const char* getBatteryIcon(uint8_t level, bool charging) const;
    
    lv_obj_t* container_ = nullptr;
    lv_obj_t* time_label_ = nullptr;
    lv_obj_t* wifi_label_ = nullptr;
    lv_obj_t* battery_label_ = nullptr;
    lv_obj_t* status_right_box_ = nullptr;
    
    void createBackButton();
    static void backButtonClickHandler(lv_event_t* e);

    lv_obj_t* back_button_ = nullptr;
    std::function<void()> back_callback_;

    bool wifi_connected_ = false;
    uint8_t battery_level_ = 100;
    bool charging_ = false;
};

} // namespace ui
