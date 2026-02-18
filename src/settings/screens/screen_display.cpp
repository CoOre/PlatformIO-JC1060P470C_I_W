#include "screen_display.h"
#include "../i18n/i18n.h"
#include "../services/display_service.h"
#include "../core/settings_store.h"
#include "../core/theme.h"
#include "ui/ui_manager.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "DisplayScreen";

// Timeout options in milliseconds
static const uint32_t TIMEOUT_OPTIONS[] = {
    15000,   // 15s
    30000,   // 30s
    60000,   // 1m
    300000,  // 5m
    0        // never
};

static const size_t TIMEOUT_COUNT = sizeof(TIMEOUT_OPTIONS) / sizeof(TIMEOUT_OPTIONS[0]);

// ============================================================================
// Lifecycle
// ============================================================================

bool DisplayScreen::create(lv_obj_t* parent) {
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
    createBrightnessSection();
    createIdleBrightnessSection();
    createTimeoutSection();
    createRotationSection();
    createThemeSection();
    
    ESP_LOGI(TAG, "Display screen created");
    return true;
}

void DisplayScreen::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    
    ESP_LOGI(TAG, "Display screen destroyed");
}

// ============================================================================
// UI Creation
// ============================================================================

void DisplayScreen::createHeader() {
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
    lv_label_set_text(title, S(DISPLAY_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, theme_.title, LV_PART_MAIN);
}

void DisplayScreen::createBrightnessSection() {
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
    
    lv_obj_t* header = lv_obj_create(section);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(header);
    lv_label_set_text(label, S(DISPLAY_BRIGHTNESS));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    brightness_label_ = lv_label_create(header);
    uint8_t current_brightness = DisplayService::instance().getBrightness();
    lv_label_set_text_fmt(brightness_label_, "%d%%", current_brightness);
    lv_obj_set_style_text_font(brightness_label_, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(brightness_label_, theme_.title, LV_PART_MAIN);
    
    brightness_slider_ = lv_slider_create(section);
    lv_obj_set_size(brightness_slider_, LV_PCT(100), 22);
    lv_obj_set_style_bg_color(brightness_slider_, theme_.accent_soft, LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightness_slider_, theme_.accent, LV_PART_INDICATOR);
    lv_obj_set_style_radius(brightness_slider_, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(brightness_slider_, 10, LV_PART_INDICATOR);
    lv_slider_set_range(brightness_slider_, 0, 100);
    lv_slider_set_value(brightness_slider_, current_brightness, LV_ANIM_OFF);
    
    lv_obj_add_event_cb(brightness_slider_, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);
}

void DisplayScreen::createIdleBrightnessSection() {
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
    
    lv_obj_t* header = lv_obj_create(section);
    lv_obj_set_size(header, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t* label = lv_label_create(header);
    lv_label_set_text(label, S(DISPLAY_IDLE_BRIGHTNESS));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    idle_brightness_label_ = lv_label_create(header);
    uint8_t current_idle = DisplayService::instance().getIdleBrightness();
    lv_label_set_text_fmt(idle_brightness_label_, "%d%%", current_idle);
    lv_obj_set_style_text_font(idle_brightness_label_, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(idle_brightness_label_, theme_.title, LV_PART_MAIN);
    
    idle_brightness_slider_ = lv_slider_create(section);
    lv_obj_set_size(idle_brightness_slider_, LV_PCT(100), 22);
    lv_obj_set_style_bg_color(idle_brightness_slider_, theme_.accent_soft, LV_PART_MAIN);
    lv_obj_set_style_bg_color(idle_brightness_slider_, theme_.accent, LV_PART_INDICATOR);
    lv_obj_set_style_radius(idle_brightness_slider_, 10, LV_PART_MAIN);
    lv_obj_set_style_radius(idle_brightness_slider_, 10, LV_PART_INDICATOR);
    lv_slider_set_range(idle_brightness_slider_, 0, 100);
    lv_slider_set_value(idle_brightness_slider_, current_idle, LV_ANIM_OFF);
    
    lv_obj_add_event_cb(idle_brightness_slider_, onIdleBrightnessChanged,
                        LV_EVENT_VALUE_CHANGED, this);
}

void DisplayScreen::createTimeoutSection() {
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
    lv_label_set_text(label, S(DISPLAY_TIMEOUT));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    timeout_dropdown_ = lv_dropdown_create(row);
    lv_obj_set_size(timeout_dropdown_, 200, 40);
    
    // Build options string
    char options[128] = {};
    const char* timeout_strs[] = {
        S(DISPLAY_TIMEOUT_15S),
        S(DISPLAY_TIMEOUT_30S),
        S(DISPLAY_TIMEOUT_1M),
        S(DISPLAY_TIMEOUT_5M),
        S(DISPLAY_TIMEOUT_NEVER)
    };
    
    for (size_t i = 0; i < TIMEOUT_COUNT; i++) {
        if (i > 0) strcat(options, "\n");
        strcat(options, timeout_strs[i]);
    }
    
    lv_dropdown_set_options(timeout_dropdown_, options);
    
    // Set current selection
    uint32_t current_timeout = DisplayService::instance().getTimeout();
    for (size_t i = 0; i < TIMEOUT_COUNT; i++) {
        if (TIMEOUT_OPTIONS[i] == current_timeout) {
            lv_dropdown_set_selected(timeout_dropdown_, i);
            break;
        }
    }
    
    lv_obj_add_event_cb(timeout_dropdown_, onTimeoutSelected, LV_EVENT_VALUE_CHANGED, this);
}

// ============================================================================
// Event Handlers
// ============================================================================

void DisplayScreen::onBrightnessChanged(lv_event_t* e) {
    DisplayScreen* screen = static_cast<DisplayScreen*>(lv_event_get_user_data(e));
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    int32_t value = lv_slider_get_value(slider);
    
    // Update label
    lv_label_set_text_fmt(screen->brightness_label_, "%d%%", value);
    
    // Update brightness (not immediate - debounced)
    DisplayService::instance().setBrightness(value, false);
}

void DisplayScreen::onIdleBrightnessChanged(lv_event_t* e) {
    DisplayScreen* screen = static_cast<DisplayScreen*>(lv_event_get_user_data(e));
    lv_obj_t* slider = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    int32_t value = lv_slider_get_value(slider);
    
    // Update label
    lv_label_set_text_fmt(screen->idle_brightness_label_, "%d%%", value);
    
    DisplayService::instance().setIdleBrightness(value);
}

void DisplayScreen::onTimeoutSelected(lv_event_t* e) {
    lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    if (selected < TIMEOUT_COUNT) {
        uint32_t timeout = TIMEOUT_OPTIONS[selected];
        DisplayService::instance().setTimeout(timeout);
    }
}

void DisplayScreen::createRotationSection() {
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
    lv_label_set_text(label, S(DISPLAY_ROTATION));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);
    
    rotation_dropdown_ = lv_dropdown_create(row);
    lv_obj_set_size(rotation_dropdown_, 200, 40);
    
    // Build options string
    char options[128] = {};
    const char* rotation_strs[] = {
        S(DISPLAY_ROTATION_0),
        S(DISPLAY_ROTATION_90),
        S(DISPLAY_ROTATION_180),
        S(DISPLAY_ROTATION_270)
    };
    
    for (size_t i = 0; i < 4; i++) {
        if (i > 0) strcat(options, "\n");
        strcat(options, rotation_strs[i]);
    }
    
    lv_dropdown_set_options(rotation_dropdown_, options);
    
    // Set current selection
    uint8_t current_rotation = SettingsStore::instance().getRotation();
    if (current_rotation < 4) {
        lv_dropdown_set_selected(rotation_dropdown_, current_rotation);
    }
    
    lv_obj_add_event_cb(rotation_dropdown_, onRotationSelected, LV_EVENT_VALUE_CHANGED, this);
}

void DisplayScreen::createThemeSection() {
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
    lv_label_set_text(label, S(DISPLAY_THEME));
    lv_obj_set_style_text_font(label, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, theme_.title, LV_PART_MAIN);

    theme_switch_ = lv_switch_create(row);
    if (SettingsStore::instance().getTheme() == ThemeMode::DARK) {
        lv_obj_add_state(theme_switch_, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(theme_switch_, onThemeToggled, LV_EVENT_VALUE_CHANGED, this);

    theme_value_label_ = lv_label_create(section);
    lv_label_set_text(theme_value_label_,
                      SettingsStore::instance().getTheme() == ThemeMode::DARK
                          ? S(DISPLAY_THEME_DARK)
                          : S(DISPLAY_THEME_LIGHT));
    lv_obj_set_style_text_color(theme_value_label_, theme_.muted, LV_PART_MAIN);
}

void DisplayScreen::onRotationSelected(lv_event_t* e) {
    lv_obj_t* dropdown = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    if (selected < 4) {
        // Save to settings
        SettingsStore::instance().setRotation(static_cast<uint8_t>(selected));
        // Apply rotation via display service
        DisplayService::instance().setRotation(static_cast<uint8_t>(selected));
    }
}

void DisplayScreen::onThemeToggled(lv_event_t* e) {
    DisplayScreen* screen = static_cast<DisplayScreen*>(lv_event_get_user_data(e));
    lv_obj_t* sw = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));

    bool dark = lv_obj_has_state(sw, LV_STATE_CHECKED);
    SettingsStore::instance().setTheme(dark ? ThemeMode::DARK : ThemeMode::LIGHT);

    if (screen && screen->theme_value_label_) {
        lv_label_set_text(screen->theme_value_label_,
                          dark ? S(DISPLAY_THEME_DARK) : S(DISPLAY_THEME_LIGHT));
    }

    ui::UIManager::instance().applyTheme();
}

} // namespace settings
