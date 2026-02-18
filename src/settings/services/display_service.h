#pragma once

#include <cstdint>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "lvgl.h"

namespace settings {

// ============================================================================
// Display Events
// ============================================================================

enum class DisplayEvent {
    BRIGHTNESS_CHANGED,
    TIMEOUT_CHANGED,
    BACKLIGHT_DIMMED,
    BACKLIGHT_OFF
};

struct DisplayEventData {
    DisplayEvent event;
    uint8_t brightness;  // For BRIGHTNESS_CHANGED
    uint32_t timeout_ms; // For TIMEOUT_CHANGED
};

// ============================================================================
// Display Service
// ============================================================================

/**
 * @brief Display backlight and timeout service
 * 
 * Features:
 * - PWM brightness control (0-100%)
 * - Configurable backlight timeout (dim → off)
 * - Activity tracking to reset timeout
 * - Smooth brightness transitions
 * 
 * Hardware: Uses LEDC PWM on configured backlight pin
 */
class DisplayService {
public:
    static DisplayService& instance();
    
    // Disable copy/move
    DisplayService(const DisplayService&) = delete;
    DisplayService& operator=(const DisplayService&) = delete;
    
    /**
     * @brief Initialize display service
     * @param backlight_pin GPIO pin for backlight PWM (from pins_config.h)
     * @param pwm_channel LEDC channel to use (0-15)
     * @param pwm_frequency PWM frequency in Hz (default 5000)
     * @param ui_event_obj LVGL object to receive events
     * @return true if successful
     */
    bool init(uint8_t backlight_pin, uint8_t pwm_channel = 0, 
              uint32_t pwm_frequency = 5000,
              lv_obj_t* ui_event_obj = nullptr);
    
    /**
     * @brief Deinitialize
     */
    void deinit();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ============================================================================
    // Event Handling
    // ============================================================================
    
    using EventCallback = std::function<void(const DisplayEventData&)>;
    
    void setEventCallback(EventCallback callback);
    void setUIEventObject(lv_obj_t* obj);
    
    // ============================================================================
    // Brightness Control
    // ============================================================================
    
    /**
     * @brief Set brightness (0-100%)
     * @param brightness 0-100
     * @param immediate If false, smooth transition
     */
    void setBrightness(uint8_t brightness, bool immediate = false);
    
    /**
     * @brief Get current brightness
     */
    uint8_t getBrightness() const;
    
    /**
     * @brief Set target brightness (for smooth transitions)
     */
    void setTargetBrightness(uint8_t brightness);
    
    // ============================================================================
    // Timeout Control
    // ============================================================================
    
    /**
     * @brief Set backlight timeout
     * @param timeout_ms Timeout in milliseconds, 0 = never
     */
    void setTimeout(uint32_t timeout_ms);
    
    /**
     * @brief Get current timeout
     */
    uint32_t getTimeout() const;
    
    // ============================================================================
    // Rotation Control
    // ============================================================================
    
    /**
     * @brief Set display rotation (0=0°, 1=90°, 2=180°, 3=270°)
     * @param rotation 0-3
     * @param lcd_ptr Pointer to LGFX instance for hardware rotation
     * @param disp_ptr Pointer to LVGL display for LVGL rotation
     */
    void setRotation(uint8_t rotation, void* lcd_ptr = nullptr, lv_display_t* disp_ptr = nullptr);
    
    /**
     * @brief Get current rotation
     */
    uint8_t getRotation() const;
    
    /**
     * @brief Get timeout as human-readable string
     */
    const char* getTimeoutString() const;
    
    // ============================================================================
    // Activity Tracking
    // ============================================================================
    
    /**
     * @brief Mark activity (resets timeout timer)
     * Call this on touch events or button presses
     */
    void markActivity();
    
    /**
     * @brief Check if backlight is currently dimmed/off
     */
    bool isDimmed() const { return current_brightness_ < target_brightness_; }
    bool isOff() const { return current_brightness_ == 0; }
    
    /**
     * @brief Wake up display (full brightness)
     */
    void wakeUp();
    
    /**
     * @brief Force backlight off (override)
     */
    void forceOff();

private:
    DisplayService() = default;
    ~DisplayService() = default;
    
    static void taskEntry(void* param);
    void runTask();
    
    static void timeoutCallback(TimerHandle_t xTimer);
    void onTimeout();
    
    void notifyEvent(const DisplayEventData& data);
    void sendLVGLEvent(const DisplayEventData& data);
    
    void applyBrightness(uint8_t brightness);
    void updateBrightness();
    
    // Hardware config
    uint8_t backlight_pin_ = 255;
    uint8_t pwm_channel_ = 0;
    uint32_t pwm_frequency_ = 5000;
    uint32_t pwm_resolution_ = 8;  // 8-bit = 0-255
    
    // State
    bool initialized_ = false;
    TaskHandle_t task_handle_ = nullptr;
    TimerHandle_t timeout_timer_ = nullptr;
    mutable SemaphoreHandle_t mutex_ = nullptr;
    
    EventCallback event_callback_;
    lv_obj_t* ui_event_obj_ = nullptr;
    
    // Brightness
    uint8_t target_brightness_ = 80;
    uint8_t current_brightness_ = 80;
    static constexpr uint8_t BRIGHTNESS_STEP = 5;
    static constexpr uint32_t TRANSITION_INTERVAL_MS = 20;
    
    // Timeout
    uint32_t timeout_ms_ = 60000;
    volatile bool activity_pending_ = false;
    
    // Dim levels
    static constexpr uint8_t DIM_BRIGHTNESS = 10;  // 10% when dimmed
    
    // Rotation
    uint8_t rotation_ = 0;  // 0=0°, 1=90°, 2=180°, 3=270°
};

} // namespace settings
