#pragma once

#include <cstdint>
#include <ctime>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"

namespace settings {

// ============================================================================
// Time Events
// ============================================================================

enum class TimeEvent {
    TIME_UPDATED,      // Local time was updated (every second)
    TIME_SYNCED,       // NTP sync completed
    TIME_SYNC_FAILED,  // NTP sync failed
    NTP_ENABLED,       // NTP was enabled
    NTP_DISABLED       // NTP was disabled
};

struct TimeEventData {
    TimeEvent event;
    time_t timestamp;
    int32_t reason;  // For failures
};

// ============================================================================
// Time Service
// ============================================================================

/**
 * @brief Time and NTP service
 * 
 * Handles:
 * - NTP synchronization
 * - Timezone management
 * - Manual time setting
 * - Periodic time updates to UI
 * 
 * Events are delivered via callback and LVGL events.
 */
class TimeService {
public:
    static TimeService& instance();
    
    // Disable copy/move
    TimeService(const TimeService&) = delete;
    TimeService& operator=(const TimeService&) = delete;
    
    /**
     * @brief Initialize time service
     * @param ui_event_obj LVGL object to receive events
     * @return true if successful
     */
    bool init(lv_obj_t* ui_event_obj = nullptr);
    
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
    
    using EventCallback = std::function<void(const TimeEventData&)>;
    
    void setEventCallback(EventCallback callback);
    void setUIEventObject(lv_obj_t* obj);
    
    // ============================================================================
    // Time Queries
    // ============================================================================
    
    /**
     * @brief Get current local time
     * @return Unix timestamp (seconds since epoch)
     */
    time_t getTimestamp() const;
    
    /**
     * @brief Get broken-down local time
     */
    struct tm getLocalTime() const;
    
    /**
     * @brief Get formatted time string
     * @param format strftime format string
     * @param buf output buffer
     * @param buf_size buffer size
     * @return true if successful
     */
    bool formatTime(const char* format, char* buf, size_t buf_size) const;
    
    /**
     * @brief Get time as formatted string (common formats)
     */
    const char* getTimeString(bool format_24h = true) const;  // "14:30" or "2:30 PM"
    const char* getDateString() const;  // "31.01.2026"
    const char* getDateTimeString() const;  // "31.01.2026 14:30"
    
    /**
     * @brief Check if time is valid (has been set)
     */
    bool isTimeValid() const;
    
    /**
     * @brief Check if time was synced from NTP
         */
    bool isSynced() const { return ntp_synced_; }
    
    // ============================================================================
    // Time Setting
    // ============================================================================
    
    /**
     * @brief Set time manually
     * @param year Full year (e.g., 2026)
     * @param month 1-12
     * @param day 1-31
     * @param hour 0-23
     * @param minute 0-59
     * @param second 0-59
     * @return true if successful
     */
    bool setManualTime(int year, int month, int day, 
                       int hour, int minute, int second);
    
    /**
     * @brief Set time from Unix timestamp
     */
    bool setTimestamp(time_t timestamp);
    
    // ============================================================================
    // Timezone
    // ============================================================================
    
    /**
     * @brief Set timezone using POSIX TZ string
     * @param tz POSIX timezone string (e.g., "MSK-3" or "CET-1CEST,M3.5.0,M10.5.0/3")
     * @return true if valid
     */
    bool setTimezone(const char* tz);
    
    /**
     * @brief Get current timezone string
     */
    const char* getTimezone() const;
    
    /**
     * @brief Get timezone offset in minutes from UTC
     */
    int getTimezoneOffsetMinutes() const;
    
    // ============================================================================
    // NTP
    // ============================================================================
    
    /**
     * @brief Enable/disable NTP
     */
    void setNtpEnabled(bool enabled);
    
    /**
     * @brief Check if NTP is enabled
     */
    bool isNtpEnabled() const;
    
    /**
     * @brief Set NTP server
     */
    void setNtpServer(const char* server);
    
    /**
     * @brief Get NTP server
     */
    const char* getNtpServer() const;
    
    /**
     * @brief Trigger immediate NTP sync
     * @return true if sync started
     */
    bool syncNtp();
    
    /**
     * @brief Request NTP sync when WiFi becomes available
     * Call this when WiFi connects to trigger immediate NTP sync
     * @return true if sync will be attempted
     */
    bool requestNtpSyncOnConnect();
    
    /**
     * @brief Get last NTP sync time
     * @return Unix timestamp or 0 if never synced
     */
    time_t getLastSyncTime() const { return last_sync_time_; }

private:
    TimeService() = default;
    ~TimeService() = default;
    
    static void taskEntry(void* param);
    void runTask();
    
    void notifyEvent(const TimeEventData& data);
    void sendLVGLEvent(const TimeEventData& data);
    
    bool initNTP();
    void deinitNTP();
    static void ntpCallback(time_t now);
    void onNtpSync(time_t now);
    
    // State
    bool initialized_ = false;
    TaskHandle_t task_handle_ = nullptr;
    mutable SemaphoreHandle_t mutex_ = nullptr;
    
    EventCallback event_callback_;
    lv_obj_t* ui_event_obj_ = nullptr;
    
    // Time state
    bool ntp_enabled_ = true;
    bool ntp_synced_ = false;
    time_t last_sync_time_ = 0;
    time_t boot_time_ = 0;
    
    // NTP config
    char ntp_server_[64] = "pool.ntp.org";
    char timezone_[64] = "Europe/Moscow";
    
    static TimeService* instance_;
};

} // namespace settings
