#include "time_service.h"
#include "../core/settings_store.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <Arduino.h>
#include <sys/time.h>

namespace settings {

static const char* TAG = "TimeService";
static constexpr uint32_t TASK_STACK_SIZE = 3072;
static constexpr UBaseType_t TASK_PRIORITY = 4;
static constexpr uint32_t UPDATE_INTERVAL_MS = 1000;  // 1 second updates

TimeService* TimeService::instance_ = nullptr;

// ============================================================================
// Singleton
// ============================================================================

TimeService& TimeService::instance() {
    static TimeService instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool TimeService::init(lv_obj_t* ui_event_obj) {
    if (initialized_) {
        return true;
    }
    
    instance_ = this;
    ui_event_obj_ = ui_event_obj;
    
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }
    
    // Load settings
    SettingsStore& store = SettingsStore::instance();
    ntp_enabled_ = store.getNtpEnabled();
    strncpy(ntp_server_, store.getNtpServer(), sizeof(ntp_server_) - 1);
    strncpy(timezone_, store.getTimezone(), sizeof(timezone_) - 1);
    
    // Set timezone
    setenv("TZ", timezone_, 1);
    tzset();
    
    // Record boot time (will be updated on NTP sync)
    boot_time_ = ::time(nullptr);
    
    // Init NTP if enabled
    if (ntp_enabled_) {
        initNTP();
    }
    
    // Create update task
    BaseType_t ret = xTaskCreate(
        taskEntry,
        "time_svc",
        TASK_STACK_SIZE,
        this,
        TASK_PRIORITY,
        &task_handle_
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        deinitNTP();
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "Time service initialized, NTP %s", ntp_enabled_ ? "enabled" : "disabled");
    
    return true;
}

void TimeService::deinit() {
    if (!initialized_) return;
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    
    deinitNTP();
    
    if (mutex_) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
    
    initialized_ = false;
    instance_ = nullptr;
    
    ESP_LOGI(TAG, "Time service deinitialized");
}

// ============================================================================
// Task
// ============================================================================

void TimeService::taskEntry(void* param) {
    TimeService* self = static_cast<TimeService*>(param);
    self->runTask();
}

void TimeService::runTask() {
    uint32_t last_update = 0;
    uint32_t last_ntp_retry = 0;
    
    while (true) {
        uint32_t now = millis();
        
        // Send time update event every second
        if (now - last_update >= UPDATE_INTERVAL_MS) {
            last_update = now;
            
            TimeEventData evt;
            evt.event = TimeEvent::TIME_UPDATED;
            evt.timestamp = ::time(nullptr);
            notifyEvent(evt);
        }
        
        // Retry NTP if not synced and enabled (every 60 seconds)
        if (ntp_enabled_ && !ntp_synced_ && (now - last_ntp_retry > 60000)) {
            last_ntp_retry = now;
            ESP_LOGI(TAG, "Retrying NTP sync...");
            esp_sntp_restart();
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Event Handling
// ============================================================================

void TimeService::setEventCallback(EventCallback callback) {
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        event_callback_ = callback;
        xSemaphoreGive(mutex_);
    }
}

void TimeService::setUIEventObject(lv_obj_t* obj) {
    ui_event_obj_ = obj;
}

void TimeService::notifyEvent(const TimeEventData& data) {
    if (event_callback_) {
        event_callback_(data);
    }
    sendLVGLEvent(data);
}

void TimeService::sendLVGLEvent(const TimeEventData& data) {
    if (!ui_event_obj_) return;
    
    TimeEventData* evt_copy = new TimeEventData(data);
    lv_obj_send_event(ui_event_obj_, LV_EVENT_VALUE_CHANGED, evt_copy);
}

// ============================================================================
// Time Queries
// ============================================================================

time_t TimeService::getTimestamp() const {
    return ::time(nullptr);
}

struct tm TimeService::getLocalTime() const {
    time_t now = ::time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return timeinfo;
}

bool TimeService::formatTime(const char* format, char* buf, size_t buf_size) const {
    if (!format || !buf || buf_size == 0) return false;
    
    time_t now = ::time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    return strftime(buf, buf_size, format, &timeinfo) > 0;
}

const char* TimeService::getTimeString(bool format_24h) const {
    static char buf[16];
    
    if (format_24h) {
        formatTime("%H:%M", buf, sizeof(buf));
    } else {
        formatTime("%I:%M %p", buf, sizeof(buf));
    }
    
    return buf;
}

const char* TimeService::getDateString() const {
    static char buf[32];
    formatTime("%d.%m.%Y", buf, sizeof(buf));
    return buf;
}

const char* TimeService::getDateTimeString() const {
    static char buf[64];
    formatTime("%d.%m.%Y %H:%M", buf, sizeof(buf));
    return buf;
}

bool TimeService::isTimeValid() const {
    time_t now = ::time(nullptr);
    // Time is valid if it's after 2024-01-01
    return now > 1704067200;
}

// ============================================================================
// Time Setting
// ============================================================================

bool TimeService::setManualTime(int year, int month, int day, 
                                 int hour, int minute, int second) {
    struct tm timeinfo = {};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    
    time_t t = mktime(&timeinfo);
    if (t == -1) {
        ESP_LOGE(TAG, "Invalid time");
        return false;
    }
    
    return setTimestamp(t);
}

bool TimeService::setTimestamp(time_t timestamp) {
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    
    if (settimeofday(&tv, nullptr) != 0) {
        ESP_LOGE(TAG, "Failed to set time");
        return false;
    }
    
    ESP_LOGI(TAG, "Time set manually: %s", getDateTimeString());
    
    // Notify
    TimeEventData evt;
    evt.event = TimeEvent::TIME_UPDATED;
    evt.timestamp = timestamp;
    notifyEvent(evt);
    
    return true;
}

// ============================================================================
// Timezone
// ============================================================================

bool TimeService::setTimezone(const char* tz) {
    if (!tz) return false;
    
    // Validate by trying to set it
    char old_tz[64];
    strncpy(old_tz, timezone_, sizeof(old_tz));
    
    if (setenv("TZ", tz, 1) != 0) {
        return false;
    }
    tzset();
    
    // Store if valid
    strncpy(timezone_, tz, sizeof(timezone_) - 1);
    timezone_[sizeof(timezone_) - 1] = '\0';
    
    SettingsStore::instance().setTimezone(timezone_);
    
    ESP_LOGI(TAG, "Timezone set to: %s", timezone_);
    
    // Notify time updated (format may have changed)
    TimeEventData evt;
    evt.event = TimeEvent::TIME_UPDATED;
    evt.timestamp = ::time(nullptr);
    notifyEvent(evt);
    
    return true;
}

const char* TimeService::getTimezone() const {
    return timezone_;
}

int TimeService::getTimezoneOffsetMinutes() const {
    struct tm timeinfo;
    time_t now = ::time(nullptr);
    localtime_r(&now, &timeinfo);
    
    // Calculate offset
    time_t local = mktime(&timeinfo);
    struct tm gmt_timeinfo;
    gmtime_r(&now, &gmt_timeinfo);
    time_t gmt = mktime(&gmt_timeinfo);
    
    return (int)((local - gmt) / 60);
}

// ============================================================================
// NTP
// ============================================================================

void TimeService::setNtpEnabled(bool enabled) {
    if (ntp_enabled_ == enabled) return;
    
    ntp_enabled_ = enabled;
    SettingsStore::instance().setNtpEnabled(enabled);
    
    if (enabled) {
        initNTP();
    } else {
        deinitNTP();
        ntp_synced_ = false;
    }
    
    TimeEventData evt;
    evt.event = enabled ? TimeEvent::NTP_ENABLED : TimeEvent::NTP_DISABLED;
    evt.timestamp = ::time(nullptr);
    notifyEvent(evt);
    
    ESP_LOGI(TAG, "NTP %s", enabled ? "enabled" : "disabled");
}

bool TimeService::isNtpEnabled() const {
    return ntp_enabled_;
}

void TimeService::setNtpServer(const char* server) {
    if (!server) return;
    
    strncpy(ntp_server_, server, sizeof(ntp_server_) - 1);
    ntp_server_[sizeof(ntp_server_) - 1] = '\0';
    
    SettingsStore::instance().setNtpServer(ntp_server_);
    
    // Restart NTP if running
    if (ntp_enabled_) {
        deinitNTP();
        initNTP();
    }
    
    ESP_LOGI(TAG, "NTP server set to: %s", ntp_server_);
}

const char* TimeService::getNtpServer() const {
    return ntp_server_;
}

bool TimeService::syncNtp() {
    if (!ntp_enabled_) return false;
    
    ESP_LOGI(TAG, "Triggering NTP sync...");
    esp_sntp_restart();
    return true;
}

bool TimeService::requestNtpSyncOnConnect() {
    if (!ntp_enabled_) {
        ESP_LOGD(TAG, "NTP sync on connect skipped - NTP disabled");
        return false;
    }
    
    // If already synced recently (within 5 minutes), don't sync again
    if (ntp_synced_ && last_sync_time_ > 0) {
        time_t now = ::time(nullptr);
        if (now - last_sync_time_ < 300) {  // 5 minutes
            ESP_LOGD(TAG, "NTP sync on connect skipped - synced recently");
            return false;
        }
    }
    
    ESP_LOGI(TAG, "Requesting NTP sync on WiFi connect...");
    
    // Reset sync state to force a new sync
    ntp_synced_ = false;
    
    // Restart SNTP to trigger immediate sync
    esp_sntp_restart();
    
    return true;
}

bool TimeService::initNTP() {
    ESP_LOGI(TAG, "Initializing SNTP with server: %s", ntp_server_);
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, ntp_server_);
    esp_sntp_set_time_sync_notification_cb([](struct timeval *tv) {
        if (instance_ && tv) {
            instance_->onNtpSync(tv->tv_sec);
        }
    });
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_init();
    
    return true;
}

void TimeService::deinitNTP() {
    esp_sntp_stop();
}

void TimeService::onNtpSync(time_t now) {
    ESP_LOGI(TAG, "NTP sync received: %s", getDateTimeString());
    
    ntp_synced_ = true;
    last_sync_time_ = now;
    
    // Notify
    TimeEventData evt;
    evt.event = TimeEvent::TIME_SYNCED;
    evt.timestamp = now;
    notifyEvent(evt);
}

} // namespace settings
