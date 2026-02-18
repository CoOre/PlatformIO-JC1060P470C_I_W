#pragma once

#include "i18n.h"

namespace settings {

// English translations
static const char* const STRINGS_EN[static_cast<size_t>(StringID::COUNT)] = {
    // Common
    [static_cast<size_t>(StringID::OK)] = "OK",
    [static_cast<size_t>(StringID::CANCEL)] = "Cancel",
    [static_cast<size_t>(StringID::BACK)] = "Back",
    [static_cast<size_t>(StringID::SAVE)] = "Save",
    [static_cast<size_t>(StringID::DELETE)] = "Delete",
    [static_cast<size_t>(StringID::CONNECT)] = "Connect",
    [static_cast<size_t>(StringID::DISCONNECT)] = "Disconnect",
    [static_cast<size_t>(StringID::FORGET)] = "Forget",
    [static_cast<size_t>(StringID::SCANNING)] = "Scanning...",
    [static_cast<size_t>(StringID::CONNECTING)] = "Connecting...",
    [static_cast<size_t>(StringID::CONNECTED)] = "Connected",
    [static_cast<size_t>(StringID::DISCONNECTED)] = "Disconnected",
    [static_cast<size_t>(StringID::ERROR)] = "Error",
    [static_cast<size_t>(StringID::LOADING)] = "Loading...",
    
    // Settings Home
    [static_cast<size_t>(StringID::SETTINGS_TITLE)] = "Settings",
    [static_cast<size_t>(StringID::SETTINGS_WIFI)] = "Wi-Fi",
    [static_cast<size_t>(StringID::SETTINGS_DISPLAY)] = "Display",
    [static_cast<size_t>(StringID::SETTINGS_TIME)] = "Time",
    [static_cast<size_t>(StringID::SETTINGS_LANGUAGE)] = "Language",
    [static_cast<size_t>(StringID::SETTINGS_SYSTEM)] = "System",

    // Apps
    [static_cast<size_t>(StringID::APP_BOX_TEST)] = "Box Test",
    [static_cast<size_t>(StringID::APP_CLOCK)] = "Clock",
    [static_cast<size_t>(StringID::APP_SETTINGS)] = "Settings",
    [static_cast<size_t>(StringID::APP_GALLERY)] = "Gallery",
    [static_cast<size_t>(StringID::APP_MUSIC)] = "Music",
    [static_cast<size_t>(StringID::APP_FILES)] = "Files",
    [static_cast<size_t>(StringID::APP_CALCULATOR)] = "Calculator",
    
    // WiFi
    [static_cast<size_t>(StringID::WIFI_TITLE)] = "Wi-Fi",
    [static_cast<size_t>(StringID::WIFI_STATUS)] = "Status",
    [static_cast<size_t>(StringID::WIFI_SCAN)] = "Scan Networks",
    [static_cast<size_t>(StringID::WIFI_SAVED_NETWORKS)] = "Saved Networks",
    [static_cast<size_t>(StringID::WIFI_ENTER_PASSWORD)] = "Enter Password",
    [static_cast<size_t>(StringID::WIFI_PASSWORD)] = "Password",
    [static_cast<size_t>(StringID::WIFI_SHOW_PASSWORD)] = "Show password",
    [static_cast<size_t>(StringID::WIFI_CONNECTING)] = "Connecting...",
    [static_cast<size_t>(StringID::WIFI_CONNECTED)] = "Connected",
    [static_cast<size_t>(StringID::WIFI_DISCONNECTED)] = "Disconnected",
    [static_cast<size_t>(StringID::WIFI_FAILED)] = "Connection failed",
    [static_cast<size_t>(StringID::WIFI_SSID)] = "Network",
    [static_cast<size_t>(StringID::WIFI_IP_ADDRESS)] = "IP Address",
    [static_cast<size_t>(StringID::WIFI_SIGNAL)] = "Signal",
    [static_cast<size_t>(StringID::WIFI_NO_NETWORKS)] = "No networks found",
    [static_cast<size_t>(StringID::WIFI_AUTO_CONNECT)] = "Auto-connect",
    
    // Display
    [static_cast<size_t>(StringID::DISPLAY_TITLE)] = "Display",
    [static_cast<size_t>(StringID::DISPLAY_BRIGHTNESS)] = "Brightness",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT)] = "Backlight timeout",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_15S)] = "15 sec",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_30S)] = "30 sec",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_1M)] = "1 min",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_5M)] = "5 min",
    [static_cast<size_t>(StringID::DISPLAY_TIMEOUT_NEVER)] = "Never",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION)] = "Screen rotation",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_0)] = "0\xc2\xb0",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_90)] = "90\xc2\xb0",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_180)] = "180\xc2\xb0",
    [static_cast<size_t>(StringID::DISPLAY_ROTATION_270)] = "270\xc2\xb0",
    
    // Time
    [static_cast<size_t>(StringID::TIME_TITLE)] = "Time",
    [static_cast<size_t>(StringID::TIME_CURRENT)] = "Current time",
    [static_cast<size_t>(StringID::TIME_DATE)] = "Date",
    [static_cast<size_t>(StringID::TIME_TIME)] = "Time",
    [static_cast<size_t>(StringID::TIME_TIMEZONE)] = "Timezone",
    [static_cast<size_t>(StringID::TIME_NTP_ENABLED)] = "NTP sync",
    [static_cast<size_t>(StringID::TIME_NTP_SERVER)] = "NTP server",
    [static_cast<size_t>(StringID::TIME_SYNC_NOW)] = "Sync now",
    [static_cast<size_t>(StringID::TIME_MANUAL_SET)] = "Set manually",
    [static_cast<size_t>(StringID::TIME_YEAR)] = "Year",
    [static_cast<size_t>(StringID::TIME_MONTH)] = "Month",
    [static_cast<size_t>(StringID::TIME_DAY)] = "Day",
    [static_cast<size_t>(StringID::TIME_HOUR)] = "Hour",
    [static_cast<size_t>(StringID::TIME_MINUTE)] = "Minute",
    [static_cast<size_t>(StringID::TIME_SECOND)] = "Second",
    
    // Language
    [static_cast<size_t>(StringID::LANGUAGE_TITLE)] = "Language",
    [static_cast<size_t>(StringID::LANGUAGE_SELECT)] = "Select language",
    [static_cast<size_t>(StringID::LANGUAGE_RUSSIAN)] = "Русский",
    [static_cast<size_t>(StringID::LANGUAGE_ENGLISH)] = "English",
    [static_cast<size_t>(StringID::LANGUAGE_FORMAT_24H)] = "24 hour",
    [static_cast<size_t>(StringID::LANGUAGE_FORMAT_12H)] = "12 hour (AM/PM)",
    
    // System
    [static_cast<size_t>(StringID::SYSTEM_TITLE)] = "System",
    [static_cast<size_t>(StringID::SYSTEM_FIRMWARE_VERSION)] = "Firmware version",
    [static_cast<size_t>(StringID::SYSTEM_UPTIME)] = "Uptime",
    [static_cast<size_t>(StringID::SYSTEM_FREE_HEAP)] = "Free memory",
    [static_cast<size_t>(StringID::SYSTEM_FREE_PSRAM)] = "Free PSRAM",
    [static_cast<size_t>(StringID::SYSTEM_RESET_SETTINGS)] = "Reset settings",
    [static_cast<size_t>(StringID::SYSTEM_REBOOT)] = "Reboot",
    [static_cast<size_t>(StringID::SYSTEM_RESET_CONFIRM)] = "Reset all settings?",
    [static_cast<size_t>(StringID::SYSTEM_REBOOT_CONFIRM)] = "Reboot device?",
};

} // namespace settings
