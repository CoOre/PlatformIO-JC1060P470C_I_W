#pragma once

#include "lvgl.h"
#include <cstdint>

namespace settings {

/**
 * @brief Main Settings Application
 * 
 * Encapsulates the entire Settings app lifecycle:
 * - Initializes all services (WiFi, Time, Display)
 * - Creates and manages UI
 * - Handles navigation
 * 
 * Usage:
 *   SettingsApp app;
 *   app.create(parent);
 *   // In main loop:
 *   app.update(millis());
 *   // On destroy:
 *   app.destroy();
 */
class SettingsApp {
public:
    SettingsApp();
    ~SettingsApp();
    
    // Disable copy/move
    SettingsApp(const SettingsApp&) = delete;
    SettingsApp& operator=(const SettingsApp&) = delete;
    
    /**
     * @brief Create the settings app on given parent
     * @param parent LVGL parent object (usually MainScreen container)
     * @return true if successful
     */
    bool create(lv_obj_t* parent);
    
    /**
     * @brief Destroy the app and free all resources
     */
    void destroy();
    
    /**
     * @brief Update app (call from main loop)
     * @param now_ms current time in milliseconds
     */
    void update(uint32_t now_ms);
    
    /**
     * @brief Check if app is active (created)
     */
    bool isActive() const { return container_ != nullptr; }
    
    /**
     * @brief Handle back button
     * @return true if handled, false if should exit app
     */
    bool handleBack();

private:
    void setupContainer(lv_obj_t* parent);
    
    lv_obj_t* container_ = nullptr;
    bool initialized_ = false;
};

/**
 * @brief Initialize all settings services
 * Call once at system startup (before any UI)
 * @return true if successful
 */
bool initSettingsServices();

/**
 * @brief Deinitialize all settings services
 * Call at system shutdown
 */
void deinitSettingsServices();

} // namespace settings
