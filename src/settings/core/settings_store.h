#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

namespace settings {

// ============================================================================
// Schema Version
// ============================================================================
static constexpr uint32_t SETTINGS_SCHEMA_VERSION = 4;

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t MAX_SAVED_NETWORKS = 5;
static constexpr size_t MAX_SSID_LEN = 32;
static constexpr size_t MAX_PASS_LEN = 64;
static constexpr size_t MAX_HOSTNAME_LEN = 32;
static constexpr size_t MAX_TIMEZONE_LEN = 64;
static constexpr size_t MAX_NTP_SERVER_LEN = 64;

enum class ThemeMode : uint8_t {
    LIGHT = 0,
    DARK = 1
};

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Saved WiFi network credentials
 */
struct SavedNetwork {
    char ssid[MAX_SSID_LEN + 1] = {};
    char password[MAX_PASS_LEN + 1] = {};
    bool auto_connect = true;
    int8_t rssi = 0;  // Last known RSSI
    
    bool isEmpty() const { return ssid[0] == '\0'; }
    void clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * @brief WiFi settings
 */
struct WiFiSettings {
    bool enabled = true;
    char hostname[MAX_HOSTNAME_LEN + 1] = "ESP32-P4";
    std::array<SavedNetwork, MAX_SAVED_NETWORKS> saved_networks;
    uint8_t saved_count = 0;
    
    void clear() {
        enabled = true;
        strncpy(hostname, "ESP32-P4", sizeof(hostname));
        for (auto& net : saved_networks) {
            net.clear();
        }
        saved_count = 0;
    }
};

/**
 * @brief Display settings
 */
struct DisplaySettings {
    uint8_t brightness = 80;              // 0-100%
    uint8_t idle_brightness = 10;         // 0-100% when idle
    uint32_t timeout_ms = 60000;          // 0 = never, else milliseconds
    uint8_t rotation = 0;                 // 0=0°, 1=90°, 2=180°, 3=270°
    ThemeMode theme = ThemeMode::LIGHT;
    
    static constexpr uint32_t TIMEOUT_15S = 15000;
    static constexpr uint32_t TIMEOUT_30S = 30000;
    static constexpr uint32_t TIMEOUT_1M = 60000;
    static constexpr uint32_t TIMEOUT_5M = 300000;
    static constexpr uint32_t TIMEOUT_NEVER = 0;
    
    // Rotation values
    static constexpr uint8_t ROTATION_0 = 0;
    static constexpr uint8_t ROTATION_90 = 1;
    static constexpr uint8_t ROTATION_180 = 2;
    static constexpr uint8_t ROTATION_270 = 3;
    
    void clear() {
        brightness = 80;
        idle_brightness = 10;
        timeout_ms = TIMEOUT_1M;
        rotation = ROTATION_0;
        theme = ThemeMode::LIGHT;
    }
};

/**
 * @brief Time settings
 */
struct TimeSettings {
    char timezone[MAX_TIMEZONE_LEN + 1] = "Europe/Moscow";
    bool ntp_enabled = true;
    char ntp_server[MAX_NTP_SERVER_LEN + 1] = "pool.ntp.org";
    int16_t manual_offset_minutes = 0;    // Offset from boot time when NTP off
    
    void clear() {
        strncpy(timezone, "Europe/Moscow", sizeof(timezone));
        ntp_enabled = true;
        strncpy(ntp_server, "pool.ntp.org", sizeof(ntp_server));
        manual_offset_minutes = 0;
    }
};

/**
 * @brief Language settings
 */
struct LanguageSettings {
    enum Language : uint8_t {
        LANG_RUSSIAN = 0,
        LANG_ENGLISH = 1,
        LANG_COUNT = 2
    };
    
    Language language = LANG_RUSSIAN;
    bool format_24h = true;
    
    void clear() {
        language = LANG_RUSSIAN;
        format_24h = true;
    }
};

/**
 * @brief Complete settings data
 */
struct SettingsData {
    uint32_t schema_version = SETTINGS_SCHEMA_VERSION;
    WiFiSettings wifi;
    DisplaySettings display;
    TimeSettings time;
    LanguageSettings language;
    
    void clear() {
        schema_version = SETTINGS_SCHEMA_VERSION;
        wifi.clear();
        display.clear();
        time.clear();
        language.clear();
    }
};

// ============================================================================
// Settings Store (Singleton)
// ============================================================================

/**
 * @brief Thread-safe settings storage with NVS persistence
 * 
 * This class is the single source of truth for all settings.
 * All reads/writes are protected by a mutex.
 * Changes are debounced and persisted via SettingsPersistence.
 */
class SettingsStore {
public:
    static SettingsStore& instance();
    
    // Disable copy/move
    SettingsStore(const SettingsStore&) = delete;
    SettingsStore& operator=(const SettingsStore&) = delete;
    
    /**
     * @brief Initialize the store, load from NVS
     * @return true if successful
     */
    bool init();
    
    /**
     * @brief Check if store is initialized
     */
    bool isInitialized() const { return initialized_; }
    
    /**
     * @brief Reset all settings to defaults
     */
    void resetToDefaults();
    
    /**
     * @brief Get current schema version
     */
    static uint32_t schemaVersion() { return SETTINGS_SCHEMA_VERSION; }
    
    /**
     * @brief Check if stored schema matches current
     */
    bool checkSchemaVersion();
    
    // ============================================================================
    // WiFi Settings
    // ============================================================================
    
    bool getWiFiEnabled() const;
    void setWiFiEnabled(bool enabled);
    
    const char* getHostname() const;
    void setHostname(const char* hostname);
    
    uint8_t getSavedNetworkCount() const;
    SavedNetwork getSavedNetwork(uint8_t index) const;
    bool addSavedNetwork(const SavedNetwork& network);
    bool removeSavedNetwork(uint8_t index);
    bool removeSavedNetworkBySsid(const char* ssid);
    int8_t findSavedNetworkBySsid(const char* ssid) const;
    bool updateSavedNetworkRssi(const char* ssid, int8_t rssi);
    
    // ============================================================================
    // Display Settings
    // ============================================================================
    
    uint8_t getBrightness() const;
    void setBrightness(uint8_t brightness);  // 0-100
    
    uint8_t getIdleBrightness() const;
    void setIdleBrightness(uint8_t brightness);  // 0-100
    
    uint32_t getDisplayTimeoutMs() const;
    void setDisplayTimeoutMs(uint32_t timeout_ms);
    
    uint8_t getRotation() const;             // 0=0°, 1=90°, 2=180°, 3=270°
    void setRotation(uint8_t rotation);

    ThemeMode getTheme() const;
    void setTheme(ThemeMode theme);
    
    // ============================================================================
    // Time Settings
    // ============================================================================
    
    const char* getTimezone() const;
    void setTimezone(const char* timezone);
    
    bool getNtpEnabled() const;
    void setNtpEnabled(bool enabled);
    
    const char* getNtpServer() const;
    void setNtpServer(const char* server);
    
    int16_t getManualOffsetMinutes() const;
    void setManualOffsetMinutes(int16_t offset);
    
    // ============================================================================
    // Language Settings
    // ============================================================================
    
    LanguageSettings::Language getLanguage() const;
    void setLanguage(LanguageSettings::Language lang);
    
    bool getFormat24h() const;
    void setFormat24h(bool format_24h);
    
    // ============================================================================
    // Raw Access (for persistence layer)
    // ============================================================================
    
    /**
     * @brief Get a copy of all settings (thread-safe)
     */
    SettingsData getSettings() const;
    
    /**
     * @brief Replace all settings (thread-safe)
     */
    void setSettings(const SettingsData& settings);
    
    /**
     * @brief Mark settings as modified (triggers debounced save)
     */
    void markModified();
    
    /**
     * @brief Force immediate save
     */
    void saveNow();

private:
    SettingsStore() = default;
    ~SettingsStore() = default;
    
    mutable SemaphoreHandle_t mutex_ = nullptr;
    SettingsData data_;
    bool initialized_ = false;
    bool modified_ = false;
    
    void lock() const;
    void unlock() const;
    
    friend class SettingsPersistence;
};

} // namespace settings
