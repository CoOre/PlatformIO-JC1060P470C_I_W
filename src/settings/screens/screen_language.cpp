#include "screen_language.h"
#include "../i18n/i18n.h"
#include "../core/settings_store.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "LanguageScreen";

// ============================================================================
// Lifecycle
// ============================================================================

bool LanguageScreen::create(lv_obj_t* parent) {
    if (!parent) return false;
    
    setupContainer(parent);
    theme_ = currentThemeColors();
    lv_obj_set_style_bg_color(container_, theme_.screen_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_row(container_, 12, LV_PART_MAIN);
    lv_obj_set_scroll_dir(container_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_AUTO);
    
    createHeader();
    createLanguageSection();
    createFormatSection();
    
    ESP_LOGI(TAG, "Language screen created");
    return true;
}

void LanguageScreen::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    
    ESP_LOGI(TAG, "Language screen destroyed");
}

// ============================================================================
// UI Creation
// ============================================================================

void LanguageScreen::createHeader() {
    header_ = lv_obj_create(container_);
    lv_obj_set_size(header_, LV_PCT(100), 56);
    lv_obj_set_style_bg_color(header_, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(header_, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(header_, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(header_, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(header_, 0, LV_PART_MAIN);
    lv_obj_set_layout(header_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(LANGUAGE_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, theme_.title, LV_PART_MAIN);
}

void LanguageScreen::createLanguageSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(section, 10, LV_PART_MAIN);
    
    lv_obj_t* row = lv_obj_create(section);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, S(LANGUAGE_SELECT));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    lang_dropdown_ = lv_dropdown_create(row);
    lv_obj_set_size(lang_dropdown_, 250, 40);
    
    // Build options
    char options[64] = {};
    strcat(options, I18n::instance().getLanguageName(Language::RUSSIAN));
    strcat(options, "\n");
    strcat(options, I18n::instance().getLanguageName(Language::ENGLISH));
    
    lv_dropdown_set_options(lang_dropdown_, options);
    
    // Set current
    Language current = I18n::instance().getLanguage();
    lv_dropdown_set_selected(lang_dropdown_, static_cast<uint8_t>(current));
    
    lv_obj_add_event_cb(lang_dropdown_, onLanguageSelected, LV_EVENT_VALUE_CHANGED, this);
}

void LanguageScreen::createFormatSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(section, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(section, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(section, 14, LV_PART_MAIN);
    lv_obj_set_style_border_width(section, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(section, theme_.card_border, LV_PART_MAIN);
    lv_obj_set_style_pad_all(section, 16, LV_PART_MAIN);
    lv_obj_set_layout(section, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
    
    lv_obj_t* title = lv_label_create(section);
    lv_label_set_text(title, S(TIME_TIME));
    lv_obj_set_style_text_font(title, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, theme_.title, LV_PART_MAIN);
    
    lv_obj_t* row = lv_obj_create(section);
    lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(row);
    lv_label_set_text(label, S(LANGUAGE_FORMAT_24H));
    lv_obj_set_style_text_color(label, theme_.muted, LV_PART_MAIN);
    
    format_switch_ = lv_switch_create(row);
    
    if (SettingsStore::instance().getFormat24h()) {
        lv_obj_add_state(format_switch_, LV_STATE_CHECKED);
    }
    
    lv_obj_add_event_cb(format_switch_, onFormatToggle, LV_EVENT_VALUE_CHANGED, this);
}

// ============================================================================
// Event Handlers
// ============================================================================

void LanguageScreen::onLanguageSelected(lv_event_t* e) {
    LanguageScreen* screen = static_cast<LanguageScreen*>(lv_event_get_user_data(e));
    lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    Language new_lang = static_cast<Language>(selected);
    
    if (new_lang != I18n::instance().getLanguage()) {
        I18n::instance().setLanguage(new_lang);
        
        // Refresh UI to show new language
        // In a full implementation, refresh all labels
        // For now, just update header
        lv_obj_t* title = lv_obj_get_child(screen->header_, 0);
        if (title) {
            lv_label_set_text(title, S(LANGUAGE_TITLE));
        }
    }
}

void LanguageScreen::onFormatToggle(lv_event_t* e) {
    lv_obj_t* sw = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    bool format_24h = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SettingsStore::instance().setFormat24h(format_24h);
}

} // namespace settings
