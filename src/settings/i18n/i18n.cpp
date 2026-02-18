#include "i18n.h"
#include "strings_ru.h"
#include "strings_en.h"
#include "../core/settings_store.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "I18n";

// Language names (in their own language)
static const char* LANGUAGE_NAMES[] = {
    "Русский",   // Russian
    "English"    // English
};

// ============================================================================
// Singleton
// ============================================================================

I18n& I18n::instance() {
    static I18n instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool I18n::init() {
    // Load language from settings
    SettingsStore& store = SettingsStore::instance();
    
    uint8_t lang_val = static_cast<uint8_t>(store.getLanguage());
    if (lang_val >= static_cast<uint8_t>(Language::COUNT)) {
        lang_val = static_cast<uint8_t>(Language::ENGLISH);
    }
    
    current_language_ = static_cast<Language>(lang_val);
    
    ESP_LOGI(TAG, "Initialized with language: %s", getCurrentLanguageName());
    return true;
}

// ============================================================================
// Language Management
// ============================================================================

void I18n::setLanguage(Language lang) {
    uint8_t lang_val = static_cast<uint8_t>(lang);
    if (lang_val >= static_cast<uint8_t>(Language::COUNT)) {
        lang_val = static_cast<uint8_t>(Language::ENGLISH);
    }
    
    Language new_lang = static_cast<Language>(lang_val);
    if (current_language_ == new_lang) {
        return;
    }
    
    current_language_ = new_lang;
    
    // Save to settings
    SettingsStore::instance().setLanguage(static_cast<LanguageSettings::Language>(lang_val));
    
    ESP_LOGI(TAG, "Language changed to: %s", getCurrentLanguageName());
}

const char* I18n::getLanguageName(Language lang) const {
    uint8_t lang_val = static_cast<uint8_t>(lang);
    if (lang_val >= static_cast<uint8_t>(Language::COUNT)) {
        return "Unknown";
    }
    return LANGUAGE_NAMES[lang_val];
}

const char* I18n::getCurrentLanguageName() const {
    return getLanguageName(current_language_);
}

// ============================================================================
// String Lookup
// ============================================================================

const char* I18n::get(StringID id) const {
    size_t idx = static_cast<size_t>(id);
    
    if (idx >= static_cast<size_t>(StringID::COUNT)) {
        return "???";
    }
    
    switch (current_language_) {
        case Language::RUSSIAN:
            return STRINGS_RU[idx];
        case Language::ENGLISH:
        default:
            return STRINGS_EN[idx];
    }
}

// ============================================================================
// Convenience Functions
// ============================================================================

const char* t_time_format() {
    bool format_24h = SettingsStore::instance().getFormat24h();
    return format_24h ? "%H:%M" : "%I:%M %p";
}

const char* t_date_format() {
    // Return date format based on language
    Language lang = I18n::instance().getLanguage();
    switch (lang) {
        case Language::RUSSIAN:
            return "%d.%m.%Y";  // DD.MM.YYYY
        case Language::ENGLISH:
        default:
            return "%m/%d/%Y";  // MM/DD/YYYY
    }
}

} // namespace settings
