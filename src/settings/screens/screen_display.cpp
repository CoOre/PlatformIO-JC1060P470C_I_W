#include "screen_display.h"
#include "../i18n/i18n.h"
#include "../services/display_service.h"
#include "../core/settings_store.h"
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
    
    createHeader();
    createBrightnessSection();
    createTimeoutSection();
    createRotationSection();
    
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
    lv_obj_set_size(header_, LV_PCT(100), 50);
    lv_obj_set_style_pad_all(header_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 0, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(DISPLAY_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_center(title);
}

void DisplayScreen::createBrightnessSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Label
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(DISPLAY_BRIGHTNESS));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Value label
    brightness_label_ = lv_label_create(section);
    uint8_t current_brightness = DisplayService::instance().getBrightness();
    lv_label_set_text_fmt(brightness_label_, "%d%%", current_brightness);
    lv_obj_align(brightness_label_, LV_ALIGN_TOP_RIGHT, 0, 0);
    
    // Slider
    brightness_slider_ = lv_slider_create(section);
    lv_obj_set_size(brightness_slider_, LV_PCT(100), 20);
    lv_obj_align(brightness_slider_, LV_ALIGN_TOP_LEFT, 0, 35);
    lv_slider_set_range(brightness_slider_, 0, 100);
    lv_slider_set_value(brightness_slider_, current_brightness, LV_ANIM_OFF);
    
    lv_obj_add_event_cb(brightness_slider_, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);
}

void DisplayScreen::createTimeoutSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Label
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(DISPLAY_TIMEOUT));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Dropdown
    timeout_dropdown_ = lv_dropdown_create(section);
    lv_obj_set_size(timeout_dropdown_, 200, 40);
    lv_obj_align(timeout_dropdown_, LV_ALIGN_TOP_LEFT, 0, 35);
    
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
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Label
    lv_obj_t* label = lv_label_create(section);
    lv_label_set_text(label, S(DISPLAY_ROTATION));
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // Dropdown
    rotation_dropdown_ = lv_dropdown_create(section);
    lv_obj_set_size(rotation_dropdown_, 200, 40);
    lv_obj_align(rotation_dropdown_, LV_ALIGN_TOP_LEFT, 0, 35);
    
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

} // namespace settings
