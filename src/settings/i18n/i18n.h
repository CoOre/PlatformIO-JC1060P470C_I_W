#pragma once

#include <cstdint>
#include <cstring>

namespace settings {

// ============================================================================
// String IDs
// ============================================================================

enum class StringID : uint16_t {
    // Common
    OK,
    CANCEL,
    BACK,
    SAVE,
    DELETE,
    CONNECT,
    DISCONNECT,
    FORGET,
    SCANNING,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    ERROR,
    LOADING,
    
    // Settings Home
    SETTINGS_TITLE,
    SETTINGS_WIFI,
    SETTINGS_DISPLAY,
    SETTINGS_TIME,
    SETTINGS_LANGUAGE,
    SETTINGS_SYSTEM,

    // Apps
    APP_BOX_TEST,
    APP_CLOCK,
    APP_SETTINGS,
    APP_GALLERY,
    APP_MUSIC,
    APP_FILES,
    APP_CALCULATOR,
    
    // WiFi
    WIFI_TITLE,
    WIFI_STATUS,
    WIFI_SCAN,
    WIFI_SAVED_NETWORKS,
    WIFI_ENTER_PASSWORD,
    WIFI_PASSWORD,
    WIFI_SHOW_PASSWORD,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED,
    WIFI_FAILED,
    WIFI_SSID,
    WIFI_IP_ADDRESS,
    WIFI_SIGNAL,
    WIFI_NO_NETWORKS,
    WIFI_AUTO_CONNECT,
    
    // Display
    DISPLAY_TITLE,
    DISPLAY_BRIGHTNESS,
    DISPLAY_IDLE_BRIGHTNESS,
    DISPLAY_TIMEOUT,
    DISPLAY_TIMEOUT_15S,
    DISPLAY_TIMEOUT_30S,
    DISPLAY_TIMEOUT_1M,
    DISPLAY_TIMEOUT_5M,
    DISPLAY_TIMEOUT_NEVER,
    DISPLAY_ROTATION,
    DISPLAY_ROTATION_0,
    DISPLAY_ROTATION_90,
    DISPLAY_ROTATION_180,
    DISPLAY_ROTATION_270,
    DISPLAY_THEME,
    DISPLAY_THEME_LIGHT,
    DISPLAY_THEME_DARK,
    
    // Time
    TIME_TITLE,
    TIME_CURRENT,
    TIME_DATE,
    TIME_TIME,
    TIME_TIMEZONE,
    TIME_NTP_ENABLED,
    TIME_NTP_SERVER,
    TIME_SYNC_NOW,
    TIME_MANUAL_SET,
    TIME_YEAR,
    TIME_MONTH,
    TIME_DAY,
    TIME_HOUR,
    TIME_MINUTE,
    TIME_SECOND,
    
    // Language
    LANGUAGE_TITLE,
    LANGUAGE_SELECT,
    LANGUAGE_RUSSIAN,
    LANGUAGE_ENGLISH,
    LANGUAGE_FORMAT_24H,
    LANGUAGE_FORMAT_12H,
    
    // System
    SYSTEM_TITLE,
    SYSTEM_FIRMWARE_VERSION,
    SYSTEM_UPTIME,
    SYSTEM_FREE_HEAP,
    SYSTEM_FREE_PSRAM,
    SYSTEM_RESET_SETTINGS,
    SYSTEM_REBOOT,
    SYSTEM_RESET_CONFIRM,
    SYSTEM_REBOOT_CONFIRM,
    
    COUNT
};

// ============================================================================
// I18n Manager
// ============================================================================

enum class Language : uint8_t {
    RUSSIAN = 0,
    ENGLISH = 1,
    COUNT = 2
};

/**
 * @brief Internationalization manager
 * 
 * Provides localized strings based on current language setting.
 * Language can be changed at runtime without reboot.
 */
class I18n {
public:
    static I18n& instance();
    
    // Disable copy/move
    I18n(const I18n&) = delete;
    I18n& operator=(const I18n&) = delete;
    
    /**
     * @brief Initialize i18n, load language from settings
     */
    bool init();
    
    /**
     * @brief Set current language
     */
    void setLanguage(Language lang);
    
    /**
     * @brief Get current language
     */
    Language getLanguage() const { return current_language_; }
    
    /**
     * @brief Get language name
     */
    const char* getLanguageName(Language lang) const;
    const char* getCurrentLanguageName() const;
    
    /**
     * @brief Get localized string
     */
    const char* get(StringID id) const;
    
    /**
     * @brief Convenience operator
     */
    const char* operator[](StringID id) const { return get(id); }

private:
    I18n() = default;
    ~I18n() = default;
    
    Language current_language_ = Language::ENGLISH;
};

// ============================================================================
// Convenience Functions
// ============================================================================

/**
 * @brief Get localized string (shorthand)
 */
inline const char* t(StringID id) {
    return I18n::instance().get(id);
}

/**
 * @brief Get localized string with current language format
 */
const char* t_time_format();
const char* t_date_format();

} // namespace settings

// ============================================================================
// C-style macros for convenience in LVGL callbacks
// ============================================================================

#define S(id) settings::t(settings::StringID::id)
