#include "ui_router.h"
#include "esp_log.h"
#include "lvgl.h"

// Include screen headers for factory
#include "../screens/screen_home.h"
#include "../screens/screen_wifi.h"
#include "../screens/screen_display.h"
#include "../screens/screen_time.h"
#include "../screens/screen_language.h"
#include "../screens/screen_system.h"

namespace settings {

static const char* TAG = "UIRouter";
static Screen* createScreenByType(ScreenType type);

// ============================================================================
// Screen Base
// ============================================================================

void Screen::setupContainer(lv_obj_t* parent) {
    container_ = lv_obj_create(parent);
    
    // Fill parent
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    
    // Remove padding and borders
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_TRANSP, LV_PART_MAIN);
    
    // Flex column layout
    lv_obj_set_flex_flow(container_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
}

// ============================================================================
// Router Singleton
// ============================================================================

UIRouter& UIRouter::instance() {
    static UIRouter instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool UIRouter::init(lv_obj_t* parent) {
    if (parent_ != nullptr) {
        return true;  // Already initialized
    }
    
    if (!parent) {
        ESP_LOGE(TAG, "Parent is null");
        return false;
    }
    
    parent_ = parent;
    
    ESP_LOGI(TAG, "Router initialized");
    return true;
}

void UIRouter::deinit() {
    // Destroy all screens
    while (!stack_.empty()) {
        Screen* screen = stack_.back();
        stack_.pop_back();
        
        if (screen) {
            screen->destroy();
            delete screen;
        }
    }
    
    parent_ = nullptr;
    
    ESP_LOGI(TAG, "Router deinitialized");
}

// ============================================================================
// Navigation
// ============================================================================

bool UIRouter::push(Screen* screen) {
    if (!parent_ || !screen) {
        delete screen;
        return false;
    }
    
    // Hide current screen
    if (!stack_.empty()) {
        Screen* current = stack_.back();
        if (current) {
            current->onHide();
            lv_obj_add_flag(current->getContainer(), LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // Create new screen
    if (!screen->create(parent_)) {
        ESP_LOGE(TAG, "Failed to create screen");
        delete screen;
        return false;
    }
    
    // Add to stack
    stack_.push_back(screen);
    screen->onShow();
    
    ESP_LOGI(TAG, "Pushed screen, stack depth: %d", stack_.size());
    return true;
}

bool UIRouter::push(ScreenType type) {
    Screen* screen = createScreenByType(type);
    if (!screen) {
        ESP_LOGE(TAG, "Failed to create screen of type %d", static_cast<int>(type));
        return false;
    }
    return push(screen);
}


bool UIRouter::pop() {
    if (stack_.size() <= 1) {
        // Don't pop the root screen
        return false;
    }
    
    // Destroy current screen
    Screen* current = stack_.back();
    stack_.pop_back();
    
    if (current) {
        current->onHide();
        current->destroy();
        delete current;
    }
    
    // Show previous screen
    if (!stack_.empty()) {
        Screen* previous = stack_.back();
        if (previous && previous->getContainer()) {
            lv_obj_clear_flag(previous->getContainer(), LV_OBJ_FLAG_HIDDEN);
            previous->onShow();
        }
    }
    
    ESP_LOGI(TAG, "Popped screen, stack depth: %d", stack_.size());
    return true;
}

void UIRouter::popToRoot() {
    while (stack_.size() > 1) {
        pop();
    }
}

bool UIRouter::replace(Screen* screen) {
    if (!parent_ || !screen) {
        delete screen;
        return false;
    }
    
    // Destroy current screen
    if (!stack_.empty()) {
        Screen* current = stack_.back();
        stack_.pop_back();
        
        if (current) {
            current->onHide();
            current->destroy();
            delete current;
        }
    }
    
    // Create new screen
    if (!screen->create(parent_)) {
        ESP_LOGE(TAG, "Failed to create screen");
        delete screen;
        return false;
    }
    
    stack_.push_back(screen);
    screen->onShow();
    
    ESP_LOGI(TAG, "Replaced screen");
    return true;
}

bool UIRouter::goBack() {
    if (stack_.empty()) {
        return false;
    }
    
    // Let current screen handle back first
    Screen* current = stack_.back();
    if (current && current->onBack()) {
        return true;
    }
    
    // Otherwise pop
    return pop();
}

// ============================================================================
// Queries
// ============================================================================

Screen* UIRouter::getCurrentScreen() const {
    if (stack_.empty()) {
        return nullptr;
    }
    return stack_.back();
}

ScreenType UIRouter::getCurrentType() const {
    Screen* current = getCurrentScreen();
    if (!current) {
        return ScreenType::NONE;
    }
    return current->getType();
}

// ============================================================================
// Update
// ============================================================================

void UIRouter::update(uint32_t now_ms) {
    Screen* current = getCurrentScreen();
    if (current) {
        current->update(now_ms);
    }
}

// ============================================================================
// Screen Factory
// ============================================================================

static Screen* createScreenByType(ScreenType type) {
    switch (type) {
        case ScreenType::HOME:
            return new HomeScreen();
        case ScreenType::WIFI:
            return new WiFiScreen();
        case ScreenType::DISPLAY:
            return new DisplayScreen();
        case ScreenType::TIME:
            return new TimeScreen();
        case ScreenType::LANGUAGE:
            return new LanguageScreen();
        case ScreenType::SYSTEM:
            return new SystemScreen();
        default:
            return nullptr;
    }
}

Screen* createScreen(ScreenType type) {
    return createScreenByType(type);
}

} // namespace settings
