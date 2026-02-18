#include "ui_main_screen.h"
#include "ui_manager.h"
#include "ui_status_bar.h"
#include "esp_log.h"

namespace ui {

MainScreen::MainScreen() = default;

MainScreen::~MainScreen() {
    if (current_app_) {
        delete current_app_;
    }
    if (clock_app_) {
        delete clock_app_;
    }
    if (settings_app_) {
        delete settings_app_;
    }
    if (launcher_) {
        delete launcher_;
    }
}

bool MainScreen::create(lv_obj_t* parent) {
    if (!parent) return false;

    setupContainer(parent);
    createLauncher();

    return true;
}

void MainScreen::setupContainer(lv_obj_t* parent) {
    // Main container fills remaining space in flex layout
    container_ = lv_obj_create(parent);
    lv_obj_set_width(container_, LV_PCT(100));
    // Use flex grow to fill remaining space instead of 100% height
    lv_obj_set_height(container_, LV_SIZE_CONTENT);
    lv_obj_set_flex_grow(container_, 1);
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);

    // Remove padding
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(container_, 0, LV_PART_MAIN);

    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);
}

void MainScreen::createLauncher() {
    launcher_ = new AppLauncher();
    launcher_->create(container_);

    // Add Box Test App
    AppInfo box_test_app;
    box_test_app.name = nullptr;
    box_test_app.name_id = settings::StringID::APP_BOX_TEST;
    box_test_app.icon = nullptr; // Using color placeholder
    box_test_app.color = 0x2196F3; // Blue
    box_test_app.onLaunch = [this]() {
        launchBoxTestApp();
    };
    launcher_->addApp(box_test_app);

    // Add Clock app
    AppInfo clock_app;
    clock_app.name = nullptr;
    clock_app.name_id = settings::StringID::APP_CLOCK;
    clock_app.icon = nullptr;
    clock_app.color = 0xFF9800; // Orange
    clock_app.onLaunch = [this]() {
        launchClockApp();
    };
    launcher_->addApp(clock_app);

    // Add Settings app
    AppInfo settings_app;
    settings_app.name = nullptr;
    settings_app.name_id = settings::StringID::APP_SETTINGS;
    settings_app.icon = nullptr;
    settings_app.color = 0x4CAF50; // Green
    settings_app.onLaunch = [this]() {
        launchSettingsApp();
    };
    launcher_->addApp(settings_app);

    // Add placeholder apps for demo
    const settings::StringID app_names[] = {
        settings::StringID::APP_GALLERY,
        settings::StringID::APP_MUSIC,
        settings::StringID::APP_FILES,
        settings::StringID::APP_CALCULATOR
    };
    uint32_t app_colors[] = {0xFF9800, 0x9C27B0, 0x607D8B, 0xF44336};

    for (int i = 0; i < 4; i++) {
        AppInfo app;
        app.name = nullptr;
        app.name_id = app_names[i];
        app.icon = nullptr;
        app.color = app_colors[i];
        app.onLaunch = []() {
            // Placeholder - just log
        };
        launcher_->addApp(app);
    }

    // Set callback when app is launched
    launcher_->setOnAppLaunch([this](const AppInfo& app) {
        if (app.onLaunch) {
            app.onLaunch();
        }
        if (on_app_launch_) {
            on_app_launch_();
        }
    });
}

void MainScreen::launchBoxTestApp() {
    // Hide launcher first
    launcher_->hide();

    // Delete any existing app first
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }

    // Destroy clock app if exists
    if (clock_app_) {
        clock_app_->destroy();
        delete clock_app_;
        clock_app_ = nullptr;
    }

    // Destroy settings app if exists
    if (settings_app_) {
        settings_app_->destroy();
        delete settings_app_;
        settings_app_ = nullptr;
    }

    // Create and show box test app
    current_app_ = new BoxTestApp();
    current_app_->create(container_);

    // Force layout update
    lv_obj_update_layout(container_);

    // Notify that we left launcher
    if (on_app_launch_) {
        on_app_launch_();
    }
}

void MainScreen::launchClockApp() {
    ESP_LOGI("MainScreen", "launchClockApp called");

    // Hide launcher first
    launcher_->hide();

    // Delete any existing box test app
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }

    // Destroy clock app if exists
    if (clock_app_) {
        clock_app_->destroy();
        delete clock_app_;
        clock_app_ = nullptr;
    }

    // Destroy settings app if exists
    if (settings_app_) {
        settings_app_->destroy();
        delete settings_app_;
        settings_app_ = nullptr;
    }

    // Create and show clock app
    clock_app_ = new ClockApp();
    bool created = clock_app_->create(container_);

    if (!created) {
        ESP_LOGE("MainScreen", "Failed to create clock app");
        delete clock_app_;
        clock_app_ = nullptr;
        // Show launcher again on failure
        launcher_->show();
        return;
    }

    ESP_LOGI("MainScreen", "Clock app created successfully");

    // Force layout update
    lv_obj_update_layout(container_);

    // Notify that we left launcher
    if (on_app_launch_) {
        on_app_launch_();
    }
}

void MainScreen::launchSettingsApp() {
    ESP_LOGI("MainScreen", "launchSettingsApp called");

    // Hide launcher first
    launcher_->hide();

    // Delete any existing box test app
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }

    // Destroy clock app if exists
    if (clock_app_) {
        clock_app_->destroy();
        delete clock_app_;
        clock_app_ = nullptr;
    }

    // Destroy settings app if exists
    if (settings_app_) {
        settings_app_->destroy();
        delete settings_app_;
        settings_app_ = nullptr;
    }

    // Create and show settings app
    settings_app_ = new settings::SettingsApp();
    bool created = settings_app_->create(container_);

    if (!created) {
        ESP_LOGE("MainScreen", "Failed to create settings app");
        delete settings_app_;
        settings_app_ = nullptr;
        // Show launcher again on failure
        launcher_->show();
        return;
    }

    ESP_LOGI("MainScreen", "Settings app created successfully");

    // Force layout update
    lv_obj_update_layout(container_);

    // Notify that we left launcher
    if (on_app_launch_) {
        on_app_launch_();
    }
}

void MainScreen::showLauncher() {
    // Destroy current app if any
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }

    // Destroy clock app if exists
    if (clock_app_) {
        clock_app_->destroy();
        delete clock_app_;
        clock_app_ = nullptr;
    }

    // Destroy settings app if exists
    if (settings_app_) {
        settings_app_->destroy();
        delete settings_app_;
        settings_app_ = nullptr;
    }

    // Show launcher
    launcher_->show();
}

bool MainScreen::isInLauncher() const {
    return launcher_ && launcher_->isVisible();
}

bool MainScreen::handleBackInSettings() {
    if (settings_app_ && settings_app_->isActive()) {
        if (settings_app_->handleBack()) {
            return true;
        }
        showLauncher();
        return true;
    }
    return false;
}

void MainScreen::update(uint32_t now_ms) {
    if (current_app_ && current_app_->isActive()) {
        current_app_->update(now_ms);
    }
    if (clock_app_ && clock_app_->isActive()) {
        clock_app_->update(now_ms);
    }
    if (settings_app_ && settings_app_->isActive()) {
        settings_app_->update(now_ms);
    }
}

void MainScreen::setOnAppLaunch(std::function<void()> callback) {
    on_app_launch_ = callback;
}

void MainScreen::applyTheme(const settings::ThemeColors& theme) {
    if (container_) {
        lv_obj_set_style_bg_color(container_, theme.screen_bg, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    }
    if (launcher_) {
        launcher_->applyTheme(theme);
    }
    if (settings_app_ && settings_app_->isActive()) {
        settings_app_->applyTheme();
    }
}

} // namespace ui
