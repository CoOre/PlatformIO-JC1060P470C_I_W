#pragma once

#include "lvgl.h"
#include <cstdint>

namespace ui {

// Forward declarations
class StatusBar;
class MainScreen;

/**
 * @brief Central UI manager class
 * 
 * Handles initialization, screen management, and global UI state.
 * Singleton pattern for global access.
 */
class UIManager {
public:
    static UIManager& instance();

    // Disable copy/move
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    /**
     * @brief Initialize the UI system
     * @return true if successful
     */
    bool init();

    /**
     * @brief Update UI (call from loop)
     * @param now_ms current time in milliseconds
     */
    void update(uint32_t now_ms);

    /**
     * @brief Get the current screen resolution
     */
    static constexpr uint16_t screenWidth() { return 1024; }
    static constexpr uint16_t screenHeight() { return 600; }

    // Access to UI components
    StatusBar* statusBar() const { return status_bar_; }
    MainScreen* mainScreen() const { return main_screen_; }

    /**
     * @brief Set touch activity state
     */
    void setTouchActive(bool active) { touch_active_ = active; }
    bool isTouchActive() const { return touch_active_; }

    /**
     * @brief Update FPS display
     */
    void updateFPS(uint32_t fps);

    /**
     * @brief Go back to launcher menu
     */
    void goBackToLauncher();

    /**
     * @brief Apply current theme to UI
     */
    void applyTheme();

private:
    UIManager() = default;
    ~UIManager() = default;

    void createFPSLabel();

    StatusBar* status_bar_ = nullptr;
    MainScreen* main_screen_ = nullptr;
    lv_obj_t* main_page_ = nullptr;

    lv_obj_t* fps_label_ = nullptr;
    bool touch_active_ = false;
    bool initialized_ = false;
};

} // namespace ui
