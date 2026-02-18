#include "screen_system.h"
#include "../i18n/i18n.h"
#include "../core/settings_store.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <Arduino.h>

namespace settings {

static const char* TAG = "SystemScreen";
static const char* FIRMWARE_VERSION = "1.0.0";

// ============================================================================
// Lifecycle
// ============================================================================

bool SystemScreen::create(lv_obj_t* parent) {
    if (!parent) return false;
    
    setupContainer(parent);
    
    createHeader();
    createInfoSection();
    createActionsSection();
    createConfirmDialog();
    
    ESP_LOGI(TAG, "System screen created");
    return true;
}

void SystemScreen::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    
    ESP_LOGI(TAG, "System screen destroyed");
}

void SystemScreen::update(uint32_t now_ms) {
    // Update info every second
    static uint32_t last_update = 0;
    if (now_ms - last_update >= 1000) {
        last_update = now_ms;
        refreshInfo();
    }
}

// ============================================================================
// UI Creation
// ============================================================================

void SystemScreen::createHeader() {
    header_ = lv_obj_create(container_);
    lv_obj_set_size(header_, LV_PCT(100), 50);
    lv_obj_set_style_pad_all(header_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 0, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(SYSTEM_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_center(title);
}

void SystemScreen::createInfoSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    int y = 0;
    
    // Version
    version_label_ = lv_label_create(section);
    lv_label_set_text_fmt(version_label_, "%s: %s", 
                          S(SYSTEM_FIRMWARE_VERSION), FIRMWARE_VERSION);
    lv_obj_align(version_label_, LV_ALIGN_TOP_LEFT, 0, y);
    y += 30;
    
    // Uptime
    uptime_label_ = lv_label_create(section);
    lv_obj_align(uptime_label_, LV_ALIGN_TOP_LEFT, 0, y);
    y += 30;
    
    // Heap
    heap_label_ = lv_label_create(section);
    lv_obj_align(heap_label_, LV_ALIGN_TOP_LEFT, 0, y);
    y += 30;
    
    // PSRAM
    psram_label_ = lv_label_create(section);
    lv_obj_align(psram_label_, LV_ALIGN_TOP_LEFT, 0, y);
    
    refreshInfo();
}

void SystemScreen::createActionsSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 15, LV_PART_MAIN);
    
    // Reset button
    lv_obj_t* reset_btn = lv_btn_create(section);
    lv_obj_set_size(reset_btn, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(0xF44336), LV_PART_MAIN);
    
    lv_obj_t* reset_lbl = lv_label_create(reset_btn);
    lv_label_set_text(reset_lbl, S(SYSTEM_RESET_SETTINGS));
    lv_obj_center(reset_lbl);
    
    lv_obj_add_event_cb(reset_btn, onResetClicked, LV_EVENT_CLICKED, this);
    
    // Reboot button
    lv_obj_t* reboot_btn = lv_btn_create(section);
    lv_obj_set_size(reboot_btn, LV_PCT(100), 50);
    lv_obj_align(reboot_btn, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(reboot_btn, lv_color_hex(0xFF9800), LV_PART_MAIN);
    
    lv_obj_t* reboot_lbl = lv_label_create(reboot_btn);
    lv_label_set_text(reboot_lbl, S(SYSTEM_REBOOT));
    lv_obj_center(reboot_lbl);
    
    lv_obj_add_event_cb(reboot_btn, onRebootClicked, LV_EVENT_CLICKED, this);
}

void SystemScreen::createConfirmDialog() {
    confirm_dialog_ = lv_obj_create(container_);
    lv_obj_set_size(confirm_dialog_, 400, 200);
    lv_obj_center(confirm_dialog_);
    lv_obj_set_style_bg_color(confirm_dialog_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_flag(confirm_dialog_, LV_OBJ_FLAG_HIDDEN);
    
    // Message label
    confirm_label_ = lv_label_create(confirm_dialog_);
    lv_obj_set_width(confirm_label_, 360);
    lv_label_set_long_mode(confirm_label_, LV_LABEL_LONG_WRAP);
    lv_obj_align(confirm_label_, LV_ALIGN_TOP_MID, 0, 20);
    
    // Buttons
    lv_obj_t* btn_container = lv_obj_create(confirm_dialog_);
    lv_obj_set_size(btn_container, 360, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btn_container, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
    
    lv_obj_t* no_btn = lv_btn_create(btn_container);
    lv_obj_set_size(no_btn, 150, 40);
    lv_obj_t* no_lbl = lv_label_create(no_btn);
    lv_label_set_text(no_lbl, S(CANCEL));
    lv_obj_center(no_lbl);
    lv_obj_add_event_cb(no_btn, onConfirmNo, LV_EVENT_CLICKED, this);
    
    lv_obj_t* yes_btn = lv_btn_create(btn_container);
    lv_obj_set_size(yes_btn, 150, 40);
    lv_obj_set_style_bg_color(yes_btn, lv_color_hex(0xF44336), LV_PART_MAIN);
    lv_obj_t* yes_lbl = lv_label_create(yes_btn);
    lv_label_set_text(yes_lbl, S(OK));
    lv_obj_center(yes_lbl);
    lv_obj_add_event_cb(yes_btn, onConfirmYes, LV_EVENT_CLICKED, this);
}

// ============================================================================
// UI Updates
// ============================================================================

void SystemScreen::refreshInfo() {
    // Uptime
    uint32_t uptime_sec = millis() / 1000;
    uint32_t hours = uptime_sec / 3600;
    uint32_t minutes = (uptime_sec % 3600) / 60;
    uint32_t seconds = uptime_sec % 60;
    
    lv_label_set_text_fmt(uptime_label_, "%s: %02d:%02d:%02d",
                          S(SYSTEM_UPTIME), hours, minutes, seconds);
    
    // Heap
    size_t free_heap = esp_get_free_heap_size();
    lv_label_set_text_fmt(heap_label_, "%s: %d KB",
                          S(SYSTEM_FREE_HEAP), free_heap / 1024);
    
    // PSRAM
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    lv_label_set_text_fmt(psram_label_, "%s: %d KB",
                          S(SYSTEM_FREE_PSRAM), free_psram / 1024);
}

void SystemScreen::showConfirmDialog(const char* message, void (*callback)()) {
    lv_label_set_text(confirm_label_, message);
    confirm_callback_ = callback;
    lv_obj_clear_flag(confirm_dialog_, LV_OBJ_FLAG_HIDDEN);
    showing_confirm_ = true;
}

void SystemScreen::hideConfirmDialog() {
    lv_obj_add_flag(confirm_dialog_, LV_OBJ_FLAG_HIDDEN);
    showing_confirm_ = false;
    confirm_callback_ = nullptr;
}

// ============================================================================
// Event Handlers
// ============================================================================

void SystemScreen::onResetClicked(lv_event_t* e) {
    SystemScreen* screen = static_cast<SystemScreen*>(lv_event_get_user_data(e));
    screen->showConfirmDialog(S(SYSTEM_RESET_CONFIRM), []() {
        ESP_LOGI(TAG, "Resetting settings...");
        SettingsStore::instance().resetToDefaults();
        ESP.restart();
    });
}

void SystemScreen::onRebootClicked(lv_event_t* e) {
    SystemScreen* screen = static_cast<SystemScreen*>(lv_event_get_user_data(e));
    screen->showConfirmDialog(S(SYSTEM_REBOOT_CONFIRM), []() {
        ESP_LOGI(TAG, "Rebooting...");
        ESP.restart();
    });
}

void SystemScreen::onConfirmYes(lv_event_t* e) {
    SystemScreen* screen = static_cast<SystemScreen*>(lv_event_get_user_data(e));
    
    void (*callback)() = screen->confirm_callback_;
    screen->hideConfirmDialog();
    
    if (callback) {
        callback();
    }
}

void SystemScreen::onConfirmNo(lv_event_t* e) {
    SystemScreen* screen = static_cast<SystemScreen*>(lv_event_get_user_data(e));
    screen->hideConfirmDialog();
}

} // namespace settings
