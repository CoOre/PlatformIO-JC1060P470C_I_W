#pragma once

#include "lvgl.h"
#include "app_launcher.h"
#include "box_test_app.h"
#include "clock_app.h"
#include "settings/settings_app.h"
#include "settings/core/theme.h"
#include <cstdint>
#include <functional>

namespace ui {

/**
 * @brief Main screen content component
 * 
 * Contains the main content area below the status bar.
 * Shows app launcher menu or active application.
 */
class MainScreen {
public:
    MainScreen();
    ~MainScreen();

    /**
     * @brief Create the main screen content on the given parent
     * @param parent LVGL parent object
     * @return true if successful
     */
    bool create(lv_obj_t* parent);

    /**
     * @brief Update screen logic (call from main loop)
     * @param now_ms current time in milliseconds
     */
    void update(uint32_t now_ms);

    /**
     * @brief Go back to launcher menu
     */
    void showLauncher();

    /**
     * @brief Check if currently in launcher menu
     */
    bool isInLauncher() const;

    /**
     * @brief Handle back action inside settings app
     * @return true if handled by settings or launcher transition
     */
    bool handleBackInSettings();

    /**
     * @brief Set callback when app is launched
     */
    void setOnAppLaunch(std::function<void()> callback);

    /**
     * @brief Apply current theme to main screen and active apps
     */
    void applyTheme(const settings::ThemeColors& theme);

    /**
     * @brief Get the container object
     */
    lv_obj_t* container() const { return container_; }

private:
    void setupContainer(lv_obj_t* parent);
    void createLauncher();
    void launchBoxTestApp();
    void launchClockApp();
    void launchSettingsApp();

    lv_obj_t* container_ = nullptr;
    AppLauncher* launcher_ = nullptr;
    BoxTestApp* current_app_ = nullptr;
    ClockApp* clock_app_ = nullptr;
    settings::SettingsApp* settings_app_ = nullptr;

    std::function<void()> on_app_launch_;
};

} // namespace ui
