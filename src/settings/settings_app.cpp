#include "settings_app.h"
#include "navigation/ui_router.h"
#include "core/settings_store.h"
#include "core/settings_persistence.h"
#include "services/wifi_service.h"
#include "services/time_service.h"
#include "services/display_service.h"
#include "i18n/i18n.h"
#include "pins_config.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "SettingsApp";

// ============================================================================
// Global Services Initialization
// ============================================================================

bool initSettingsServices() {
    ESP_LOGI(TAG, "Initializing settings services...");
    
    // Initialize settings store first
    if (!SettingsStore::instance().init()) {
        ESP_LOGE(TAG, "Failed to init settings store");
        return false;
    }
    
    // Initialize persistence (debounced save)
    if (!SettingsPersistence::instance().init(2000)) {
        ESP_LOGE(TAG, "Failed to init persistence");
        return false;
    }
    
    // Initialize i18n
    if (!I18n::instance().init()) {
        ESP_LOGE(TAG, "Failed to init i18n");
        return false;
    }
    
    // Initialize WiFi service
    if (!WiFiService::instance().init()) {
        ESP_LOGE(TAG, "Failed to init WiFi service");
        return false;
    }
    
    // Initialize time service
    if (!TimeService::instance().init()) {
        ESP_LOGE(TAG, "Failed to init time service");
        return false;
    }
    
    // Initialize display service (using LCD_LED pin from pins_config.h)
    if (!DisplayService::instance().init(LCD_LED, 0, 5000, nullptr)) {
        ESP_LOGE(TAG, "Failed to init display service");
        return false;
    }
    
    ESP_LOGI(TAG, "All settings services initialized");
    return true;
}

void deinitSettingsServices() {
    ESP_LOGI(TAG, "Deinitializing settings services...");
    
    DisplayService::instance().deinit();
    TimeService::instance().deinit();
    WiFiService::instance().deinit();
    SettingsPersistence::instance().deinit();
    
    ESP_LOGI(TAG, "Settings services deinitialized");
}

// ============================================================================
// SettingsApp Class
// ============================================================================

SettingsApp::SettingsApp() = default;

SettingsApp::~SettingsApp() {
    destroy();
}

bool SettingsApp::create(lv_obj_t* parent) {
    if (!parent) {
        ESP_LOGE(TAG, "Parent is null");
        return false;
    }

    if (container_) {
        ESP_LOGW(TAG, "Already created");
        return true;
    }

    ESP_LOGI(TAG, "Creating settings app...");

    // Setup container
    setupContainer(parent);
    if (!container_) {
        ESP_LOGE(TAG, "Failed to create container");
        return false;
    }

    ESP_LOGI(TAG, "Container created, initializing router...");

    // Initialize router
    if (!UIRouter::instance().init(container_)) {
        ESP_LOGE(TAG, "Failed to init router");
        destroy();
        return false;
    }

    ESP_LOGI(TAG, "Router initialized, pushing home screen...");

    // Push home screen
    if (!UIRouter::instance().push(ScreenType::HOME)) {
        ESP_LOGE(TAG, "Failed to push home screen");
        destroy();
        return false;
    }

    initialized_ = true;
    ESP_LOGI(TAG, "Settings app created successfully");
    return true;
}

void SettingsApp::destroy() {
    if (!container_) return;
    
    // Deinit router (destroys all screens)
    UIRouter::instance().deinit();
    
    // Delete container
    lv_obj_del(container_);
    container_ = nullptr;
    
    initialized_ = false;
    
    ESP_LOGI(TAG, "Settings app destroyed");
}

void SettingsApp::update(uint32_t now_ms) {
    if (!initialized_) return;
    
    // Update router (current screen)
    UIRouter::instance().update(now_ms);
}

bool SettingsApp::handleBack() {
    if (!initialized_) return false;
    
    return UIRouter::instance().goBack();
}

void SettingsApp::setupContainer(lv_obj_t* parent) {
    container_ = lv_obj_create(parent);
    
    // Fill parent completely
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    
    // Remove all padding and borders
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    
    // White background
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
}

} // namespace settings
