#pragma once

#include <cstdint>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

namespace settings {

/**
 * @brief Debounced persistence manager
 * 
 * Accumulates settings changes and saves to NVS after a delay.
 * This prevents excessive flash writes during rapid changes (e.g., slider dragging).
 * 
 * Uses a dedicated task for actual save operations to avoid stack overflow
 * in the timer service task (which has limited stack).
 * 
 * Usage:
 *   - SettingsStore calls markModified() on any change
 *   - Persistence waits for DEBOUNCE_MS of inactivity
 *   - Then signals the save task to perform the actual save
 *   - Immediate save available via saveNow()
 */
class SettingsPersistence {
public:
    static SettingsPersistence& instance();
    
    // Disable copy/move
    SettingsPersistence(const SettingsPersistence&) = delete;
    SettingsPersistence& operator=(const SettingsPersistence&) = delete;
    
    /**
     * @brief Initialize the persistence manager
     * @param debounce_ms Delay before auto-save (default 2000ms)
     * @return true if successful
     */
    bool init(uint32_t debounce_ms = 2000);
    
    /**
     * @brief Shutdown and cleanup
     */
    void deinit();
    
    /**
     * @brief Mark settings as modified (triggers debounced save)
     */
    void markModified();
    
    /**
     * @brief Force immediate save, cancel any pending debounced save
     */
    void saveNow();
    
    /**
     * @brief Check if a save is currently pending
     */
    bool isSavePending() const;
    
    /**
     * @brief Get time until next auto-save (ms), 0 if not pending
     */
    uint32_t getTimeUntilSave() const;
    
    /**
     * @brief Set debounce delay
     */
    void setDebounceMs(uint32_t debounce_ms);
    
    /**
     * @brief Get debounce delay
     */
    uint32_t getDebounceMs() const { return debounce_ms_; }

private:
    SettingsPersistence() = default;
    ~SettingsPersistence() = default;
    
    static void timerCallback(TimerHandle_t xTimer);
    void onTimer();
    
    static void saveTaskEntry(void* param);
    void runSaveTask();
    void triggerSave();
    
    // Timer for debouncing
    TimerHandle_t timer_ = nullptr;
    uint32_t debounce_ms_ = 2000;
    volatile bool save_pending_ = false;
    mutable SemaphoreHandle_t mutex_ = nullptr;
    
    // Save task (to avoid stack overflow in timer task)
    TaskHandle_t save_task_ = nullptr;
    QueueHandle_t save_queue_ = nullptr;
    volatile bool task_running_ = false;
    
    // Command types for save queue
    enum class SaveCommand : uint8_t {
        SAVE = 0,
        SHUTDOWN = 1
    };
};

/**
 * @brief RAII guard for batching multiple settings changes
 * 
 * Usage:
 *   {
 *       SettingsBatchGuard guard;
 *       store.setBrightness(50);
 *       store.setTimeout(30000);
 *       // save happens once when guard goes out of scope
 *   }
 */
class SettingsBatchGuard {
public:
    SettingsBatchGuard();
    ~SettingsBatchGuard();
    
    // Disable copy/move
    SettingsBatchGuard(const SettingsBatchGuard&) = delete;
    SettingsBatchGuard& operator=(const SettingsBatchGuard&) = delete;
    
    /**
     * @brief Force immediate save before destruction
     */
    void commitNow();

private:
    bool committed_ = false;
};

} // namespace settings
