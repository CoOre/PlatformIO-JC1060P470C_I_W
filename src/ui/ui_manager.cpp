#include "ui_manager.h"
#include "ui_status_bar.h"
#include "ui_main_screen.h"
#include "../settings/services/wifi_service.h"
#include "../settings/services/time_service.h"
#include "../settings/core/theme.h"

namespace ui {

UIManager& UIManager::instance() {
    static UIManager instance;
    return instance;
}

bool UIManager::init() {
    if (initialized_) return true;

    // Get active screen
    lv_obj_t* screen = lv_screen_active();
    if (!screen) return false;

    // Create main page container (fills entire screen)
    main_page_ = lv_obj_create(screen);
    lv_obj_set_size(main_page_, LV_PCT(100), LV_PCT(100));
    lv_obj_align(main_page_, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(main_page_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(main_page_, 0, LV_PART_MAIN);
    lv_obj_set_layout(main_page_, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_page_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_page_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(main_page_, 0, LV_PART_MAIN);

    // Create status bar (first in flex layout)
    status_bar_ = new StatusBar();
    if (!status_bar_->create(main_page_)) {
        delete status_bar_;
        status_bar_ = nullptr;
        return false;
    }

    // Create main screen content (fills remaining space)
    main_screen_ = new MainScreen();
    if (!main_screen_->create(main_page_)) {
        delete main_screen_;
        main_screen_ = nullptr;
        delete status_bar_;
        status_bar_ = nullptr;
        return false;
    }

    // Set up back button callback
    status_bar_->setBackButtonCallback([this]() {
        if (main_screen_ && main_screen_->handleBackInSettings()) {
            if (main_screen_->isInLauncher() && status_bar_) {
                status_bar_->setBackButtonVisible(false);
            }
            return;
        }
        goBackToLauncher();
    });

    // Set up app launch callback to show back button
    main_screen_->setOnAppLaunch([this]() {
        status_bar_->setBackButtonVisible(true);
    });

    // Register WiFi event callback to update status bar and trigger NTP sync
    settings::WiFiService::instance().setEventCallback([this](const settings::WiFiEventData& evt) {
        if (!status_bar_) return;
        
        switch (evt.event) {
            case settings::WiFiEvent::CONNECTED:
            case settings::WiFiEvent::IP_ASSIGNED:
                status_bar_->setWiFiConnected(true);
                break;
            case settings::WiFiEvent::DISCONNECTED:
            case settings::WiFiEvent::CONNECTION_FAILED:
                status_bar_->setWiFiConnected(false);
                break;
            default:
                break;
        }
        
        // Trigger NTP sync when WiFi is connected and IP is assigned
        if (evt.event == settings::WiFiEvent::IP_ASSIGNED) {
            settings::TimeService::instance().requestNtpSyncOnConnect();
        }
    });

    // Create FPS label (on top layer)
    createFPSLabel();
    applyTheme();

    initialized_ = true;
    return true;
}

void UIManager::createFPSLabel() {
    fps_label_ = lv_label_create(lv_layer_top());
    lv_obj_set_pos(fps_label_, 10, StatusBar::height() + 10);
    lv_obj_set_style_text_font(fps_label_, &lv_font_roboto_14, LV_PART_MAIN);
    lv_label_set_text(fps_label_, "FPS: --");
}

void UIManager::update(uint32_t now_ms) {
    if (!initialized_) return;

    // Update status bar every second
    static uint32_t last_status_update = 0;
    if (now_ms - last_status_update >= 1000) {
        last_status_update = now_ms;
        if (status_bar_) {
            status_bar_->refresh(now_ms);
        }
    }

    // Update main screen
    if (main_screen_) {
        main_screen_->update(now_ms);
    }
}

void UIManager::updateFPS(uint32_t fps) {
    if (fps_label_) {
        lv_label_set_text_fmt(fps_label_, "FPS: %u", static_cast<unsigned>(fps));
    }
}

void UIManager::goBackToLauncher() {
    if (main_screen_) {
        main_screen_->showLauncher();
    }
    if (status_bar_) {
        status_bar_->setBackButtonVisible(false);
    }
}

void UIManager::applyTheme() {
    settings::ThemeColors theme = settings::currentThemeColors();

    if (main_page_) {
        lv_obj_set_style_bg_color(main_page_, theme.screen_bg, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(main_page_, LV_OPA_100, LV_PART_MAIN);
    }

    if (status_bar_) {
        status_bar_->applyTheme(theme);
    }

    if (main_screen_) {
        main_screen_->applyTheme(theme);
    }

    if (fps_label_) {
        lv_obj_set_style_text_color(fps_label_, theme.accent, LV_PART_MAIN);
    }
}

} // namespace ui
