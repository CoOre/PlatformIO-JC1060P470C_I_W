#include "settings_store.h"
#include "settings_persistence.h"
#include <Preferences.h>
#include "esp_log.h"

namespace settings {

static const char* TAG = "SettingsStore";
static const char* NVS_NAMESPACE = "settings";
static const char* NVS_KEY_DATA = "data_v1";

// ============================================================================
// Singleton
// ============================================================================

SettingsStore& SettingsStore::instance() {
    static SettingsStore instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool SettingsStore::init() {
    if (initialized_) {
        return true;
    }
    
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }
    
    // Try to load from NVS
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {  // Read-only
        ESP_LOGW(TAG, "NVS namespace not found, using defaults");
        data_.clear();
        initialized_ = true;
        return true;
    }
    
    if (prefs.isKey(NVS_KEY_DATA)) {
        size_t len = prefs.getBytesLength(NVS_KEY_DATA);
        if (len == sizeof(SettingsData)) {
            SettingsData loaded;
            prefs.getBytes(NVS_KEY_DATA, &loaded, sizeof(loaded));
            
            if (loaded.schema_version == SETTINGS_SCHEMA_VERSION) {
                lock();
                data_ = loaded;
                unlock();
                ESP_LOGI(TAG, "Settings loaded from NVS");
            } else {
                ESP_LOGW(TAG, "Schema version mismatch: stored=%d, current=%d",
                         loaded.schema_version, SETTINGS_SCHEMA_VERSION);
                data_.clear();
                markModified();  // Will trigger save with new schema
            }
        } else {
            ESP_LOGW(TAG, "Settings size mismatch: expected=%d, got=%d",
                     sizeof(SettingsData), len);
            data_.clear();
            markModified();
        }
    } else {
        ESP_LOGI(TAG, "No settings in NVS, using defaults");
        data_.clear();
        markModified();
    }
    
    prefs.end();
    initialized_ = true;
    return true;
}

void SettingsStore::resetToDefaults() {
    lock();
    data_.clear();
    modified_ = true;
    unlock();
    
    // Clear NVS
    Preferences prefs;
    if (prefs.begin(NVS_NAMESPACE, false)) {
        prefs.clear();
        prefs.end();
    }
    
    ESP_LOGI(TAG, "Settings reset to defaults");
}

bool SettingsStore::checkSchemaVersion() {
    lock();
    bool match = (data_.schema_version == SETTINGS_SCHEMA_VERSION);
    unlock();
    return match;
}

// ============================================================================
// Thread Safety
// ============================================================================

void SettingsStore::lock() const {
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
    }
}

void SettingsStore::unlock() const {
    if (mutex_) {
        xSemaphoreGive(mutex_);
    }
}

// ============================================================================
// Persistence Interface
// ============================================================================

SettingsData SettingsStore::getSettings() const {
    lock();
    SettingsData copy = data_;
    unlock();
    return copy;
}

void SettingsStore::setSettings(const SettingsData& settings) {
    lock();
    data_ = settings;
    modified_ = true;
    unlock();
}

void SettingsStore::markModified() {
    lock();
    modified_ = true;
    unlock();
}

void SettingsStore::saveNow() {
    SettingsData to_save;
    
    lock();
    to_save = data_;
    modified_ = false;
    unlock();
    
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        ESP_LOGE(TAG, "Failed to open NVS for writing");
        return;
    }
    
    size_t written = prefs.putBytes(NVS_KEY_DATA, &to_save, sizeof(to_save));
    prefs.end();
    
    if (written == sizeof(to_save)) {
        ESP_LOGI(TAG, "Settings saved to NVS");
    } else {
        ESP_LOGE(TAG, "Failed to save settings: written=%d", written);
    }
}

// ============================================================================
// WiFi Settings
// ============================================================================

bool SettingsStore::getWiFiEnabled() const {
    lock();
    bool val = data_.wifi.enabled;
    unlock();
    return val;
}

void SettingsStore::setWiFiEnabled(bool enabled) {
    lock();
    if (data_.wifi.enabled != enabled) {
        data_.wifi.enabled = enabled;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

const char* SettingsStore::getHostname() const {
    // Note: This returns a pointer to internal data.
    // Caller must use immediately or copy.
    // For thread safety, better to use getSettings() for atomic access.
    lock();
    // This is unsafe - returning pointer while holding lock
    // Better approach: copy to buffer
    static char hostname[MAX_HOSTNAME_LEN + 1];
    strncpy(hostname, data_.wifi.hostname, sizeof(hostname));
    unlock();
    return hostname;
}

void SettingsStore::setHostname(const char* hostname) {
    if (!hostname) return;
    
    lock();
    if (strncmp(data_.wifi.hostname, hostname, MAX_HOSTNAME_LEN) != 0) {
        strncpy(data_.wifi.hostname, hostname, MAX_HOSTNAME_LEN);
        data_.wifi.hostname[MAX_HOSTNAME_LEN] = '\0';
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

uint8_t SettingsStore::getSavedNetworkCount() const {
    lock();
    uint8_t count = data_.wifi.saved_count;
    unlock();
    return count;
}

SavedNetwork SettingsStore::getSavedNetwork(uint8_t index) const {
    SavedNetwork net;
    net.clear();
    
    lock();
    if (index < MAX_SAVED_NETWORKS && index < data_.wifi.saved_count) {
        net = data_.wifi.saved_networks[index];
    }
    unlock();
    return net;
}

bool SettingsStore::addSavedNetwork(const SavedNetwork& network) {
    if (network.isEmpty()) return false;
    
    lock();
    
    // Check if already exists
    for (uint8_t i = 0; i < data_.wifi.saved_count; i++) {
        if (strcmp(data_.wifi.saved_networks[i].ssid, network.ssid) == 0) {
            // Update existing
            data_.wifi.saved_networks[i] = network;
            modified_ = true;
            unlock();
            SettingsPersistence::instance().markModified();
            return true;
        }
    }
    
    // Add new
    if (data_.wifi.saved_count < MAX_SAVED_NETWORKS) {
        data_.wifi.saved_networks[data_.wifi.saved_count] = network;
        data_.wifi.saved_count++;
        modified_ = true;
        unlock();
        SettingsPersistence::instance().markModified();
        return true;
    }
    
    unlock();
    return false;  // Full
}

bool SettingsStore::removeSavedNetwork(uint8_t index) {
    lock();
    
    if (index >= data_.wifi.saved_count) {
        unlock();
        return false;
    }
    
    // Shift remaining
    for (uint8_t i = index; i < data_.wifi.saved_count - 1; i++) {
        data_.wifi.saved_networks[i] = data_.wifi.saved_networks[i + 1];
    }
    
    data_.wifi.saved_networks[data_.wifi.saved_count - 1].clear();
    data_.wifi.saved_count--;
    modified_ = true;
    
    unlock();
    SettingsPersistence::instance().markModified();
    return true;
}

bool SettingsStore::removeSavedNetworkBySsid(const char* ssid) {
    if (!ssid) return false;
    
    lock();
    
    for (uint8_t i = 0; i < data_.wifi.saved_count; i++) {
        if (strcmp(data_.wifi.saved_networks[i].ssid, ssid) == 0) {
            unlock();
            return removeSavedNetwork(i);
        }
    }
    
    unlock();
    return false;
}

int8_t SettingsStore::findSavedNetworkBySsid(const char* ssid) const {
    if (!ssid) return -1;
    
    lock();
    
    for (uint8_t i = 0; i < data_.wifi.saved_count; i++) {
        if (strcmp(data_.wifi.saved_networks[i].ssid, ssid) == 0) {
            unlock();
            return i;
        }
    }
    
    unlock();
    return -1;
}

bool SettingsStore::updateSavedNetworkRssi(const char* ssid, int8_t rssi) {
    if (!ssid) return false;
    
    lock();
    
    for (uint8_t i = 0; i < data_.wifi.saved_count; i++) {
        if (strcmp(data_.wifi.saved_networks[i].ssid, ssid) == 0) {
            data_.wifi.saved_networks[i].rssi = rssi;
            // Don't mark modified - RSSI is transient
            unlock();
            return true;
        }
    }
    
    unlock();
    return false;
}

// ============================================================================
// Display Settings
// ============================================================================

uint8_t SettingsStore::getBrightness() const {
    lock();
    uint8_t val = data_.display.brightness;
    unlock();
    return val;
}

void SettingsStore::setBrightness(uint8_t brightness) {
    brightness = brightness > 100 ? 100 : brightness;
    
    lock();
    if (data_.display.brightness != brightness) {
        data_.display.brightness = brightness;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

uint32_t SettingsStore::getDisplayTimeoutMs() const {
    lock();
    uint32_t val = data_.display.timeout_ms;
    unlock();
    return val;
}

void SettingsStore::setDisplayTimeoutMs(uint32_t timeout_ms) {
    lock();
    if (data_.display.timeout_ms != timeout_ms) {
        data_.display.timeout_ms = timeout_ms;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

uint8_t SettingsStore::getRotation() const {
    lock();
    uint8_t val = data_.display.rotation;
    unlock();
    return val;
}

void SettingsStore::setRotation(uint8_t rotation) {
    // Clamp to valid range 0-3
    if (rotation > 3) rotation = 0;
    
    lock();
    if (data_.display.rotation != rotation) {
        data_.display.rotation = rotation;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

// ============================================================================
// Time Settings
// ============================================================================

const char* SettingsStore::getTimezone() const {
    static char tz[MAX_TIMEZONE_LEN + 1];
    lock();
    strncpy(tz, data_.time.timezone, sizeof(tz));
    unlock();
    return tz;
}

void SettingsStore::setTimezone(const char* timezone) {
    if (!timezone) return;
    
    lock();
    if (strncmp(data_.time.timezone, timezone, MAX_TIMEZONE_LEN) != 0) {
        strncpy(data_.time.timezone, timezone, MAX_TIMEZONE_LEN);
        data_.time.timezone[MAX_TIMEZONE_LEN] = '\0';
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

bool SettingsStore::getNtpEnabled() const {
    lock();
    bool val = data_.time.ntp_enabled;
    unlock();
    return val;
}

void SettingsStore::setNtpEnabled(bool enabled) {
    lock();
    if (data_.time.ntp_enabled != enabled) {
        data_.time.ntp_enabled = enabled;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

const char* SettingsStore::getNtpServer() const {
    static char server[MAX_NTP_SERVER_LEN + 1];
    lock();
    strncpy(server, data_.time.ntp_server, sizeof(server));
    unlock();
    return server;
}

void SettingsStore::setNtpServer(const char* server) {
    if (!server) return;
    
    lock();
    if (strncmp(data_.time.ntp_server, server, MAX_NTP_SERVER_LEN) != 0) {
        strncpy(data_.time.ntp_server, server, MAX_NTP_SERVER_LEN);
        data_.time.ntp_server[MAX_NTP_SERVER_LEN] = '\0';
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

int16_t SettingsStore::getManualOffsetMinutes() const {
    lock();
    int16_t val = data_.time.manual_offset_minutes;
    unlock();
    return val;
}

void SettingsStore::setManualOffsetMinutes(int16_t offset) {
    lock();
    if (data_.time.manual_offset_minutes != offset) {
        data_.time.manual_offset_minutes = offset;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

// ============================================================================
// Language Settings
// ============================================================================

LanguageSettings::Language SettingsStore::getLanguage() const {
    lock();
    auto val = data_.language.language;
    unlock();
    return val;
}

void SettingsStore::setLanguage(LanguageSettings::Language lang) {
    lock();
    if (data_.language.language != lang) {
        data_.language.language = lang;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

bool SettingsStore::getFormat24h() const {
    lock();
    bool val = data_.language.format_24h;
    unlock();
    return val;
}

void SettingsStore::setFormat24h(bool format_24h) {
    lock();
    if (data_.language.format_24h != format_24h) {
        data_.language.format_24h = format_24h;
        modified_ = true;
    }
    unlock();
    SettingsPersistence::instance().markModified();
}

} // namespace settings
