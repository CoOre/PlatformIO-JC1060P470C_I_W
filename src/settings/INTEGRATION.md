# Settings App Integration Guide

## Overview

This document describes how to integrate the Settings app into your ESP32-P4 + LVGL project.

## File Structure

```
src/
├── settings/
│   ├── ARCHITECTURE.md           # Architecture documentation
│   ├── INTEGRATION.md            # This file
│   ├── settings_app.h/.cpp       # Main app class
│   │
│   ├── core/                     # Core infrastructure
│   │   ├── settings_store.h/.cpp
│   │   └── settings_persistence.h/.cpp
│   │
│   ├── services/                 # Background services
│   │   ├── wifi_service.h/.cpp
│   │   ├── time_service.h/.cpp
│   │   └── display_service.h/.cpp
│   │
│   ├── i18n/                     # Localization
│   │   ├── i18n.h/.cpp
│   │   ├── strings_ru.h
│   │   └── strings_en.h
│   │
│   ├── navigation/               # UI navigation
│   │   ├── ui_router.h/.cpp
│   │   └── screen_base.h (in ui_router.h)
│   │
│   └── screens/                  # UI screens
│       ├── screen_home.h/.cpp
│       ├── screen_wifi.h/.cpp
│       ├── screen_display.h/.cpp
│       ├── screen_time.h/.cpp
│       ├── screen_language.h/.cpp
│       └── screen_system.h/.cpp
```

## Integration Steps

### 1. Add Source Files to Build

Add the following to your `platformio.ini` or CMakeLists.txt:

```ini
# platformio.ini
src_filter = 
    +<settings/core/*.cpp>
    +<settings/services/*.cpp>
    +<settings/i18n/*.cpp>
    +<settings/navigation/*.cpp>
    +<settings/screens/*.cpp>
    +<settings/*.cpp>
```

Or for CMake:
```cmake
file(GLOB SETTINGS_SOURCES 
    "src/settings/core/*.cpp"
    "src/settings/services/*.cpp"
    "src/settings/i18n/*.cpp"
    "src/settings/navigation/*.cpp"
    "src/settings/screens/*.cpp"
    "src/settings/*.cpp"
)
target_sources(your_target PRIVATE ${SETTINGS_SOURCES})
target_include_directories(your_target PRIVATE "src")
```

### 2. Modify main.cpp

Add at the top:
```cpp
#include "settings/settings_app.h"
#include "settings/services/display_service.h"
```

In `setup()`, after LVGL init:
```cpp
void setup() {
    // ... existing init code ...
    
    // Initialize settings services
    if (!settings::initSettingsServices()) {
        ESP_LOGE("MAIN", "Failed to init settings services");
    }
    
    // ... rest of setup ...
}
```

### 3. Modify ui_main_screen.cpp

Add include:
```cpp
#include "settings/settings_app.h"
```

Add to `MainScreen` class (in header):
```cpp
private:
    void launchSettingsApp();
    settings::SettingsApp* settings_app_ = nullptr;
```

In `createLauncher()`:
```cpp
void MainScreen::createLauncher() {
    launcher_ = new AppLauncher();
    launcher_->create(container_);

    // Existing apps...
    
    // Settings app
    AppInfo settings_app;
    settings_app.name = "Settings";  // Or use i18n
    settings_app.icon = nullptr;
    settings_app.color = 0x4CAF50;  // Green
    settings_app.onLaunch = [this]() {
        launchSettingsApp();
    };
    launcher_->addApp(settings_app);
}
```

Add launch method:
```cpp
void MainScreen::launchSettingsApp() {
    launcher_->hide();
    
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }
    
    settings_app_ = new settings::SettingsApp();
    settings_app_->create(container_);
    
    lv_obj_update_layout(container_);
    
    if (on_app_launch_) {
        on_app_launch_();
    }
}
```

Modify `showLauncher()`:
```cpp
void MainScreen::showLauncher() {
    if (settings_app_) {
        settings_app_->destroy();
        delete settings_app_;
        settings_app_ = nullptr;
    }
    
    if (current_app_) {
        current_app_->destroy();
        delete current_app_;
        current_app_ = nullptr;
    }
    
    launcher_->show();
}
```

Modify `update()`:
```cpp
void MainScreen::update(uint32_t now_ms) {
    if (settings_app_ && settings_app_->isActive()) {
        settings_app_->update(now_ms);
    } else if (current_app_ && current_app_->isActive()) {
        current_app_->update(now_ms);
    }
}
```

### 4. Modify StatusBar Back Button

In `ui_status_bar.cpp`, modify the back button handler:
```cpp
void StatusBar::onBackClicked(lv_event_t* e) {
    // Check if settings app is active and handle back
    MainScreen* main_screen = ui::UIManager::instance().mainScreen();
    if (main_screen) {
        // This will be handled by MainScreen
        main_screen->handleBackInSettings();
    }
    
    UIManager::instance().goBackToLauncher();
}
```

Add to `MainScreen`:
```cpp
bool MainScreen::handleBackInSettings() {
    if (settings_app_ && settings_app_->isActive()) {
        if (settings_app_->handleBack()) {
            return true;  // Settings app handled it
        }
        // Settings app didn't handle it, destroy it
        showLauncher();
        return true;
    }
    return false;
}
```

### 5. Update pins_config.h (if needed)

The Settings app uses `LCD_LED` for backlight control. Ensure it's defined:

```cpp
// pins_config.h
#define LCD_LED 23  // Or your actual backlight pin
```

If your backlight pin is different, modify in `settings_app.cpp`:
```cpp
// Change LCD_LED to your actual pin
DisplayService::instance().init(YOUR_BACKLIGHT_PIN, 0, 5000, nullptr);
```

### 6. Touch Activity for Backlight

Add to your touch handler in `main.cpp`:
```cpp
static void my_touchpad_read(lv_indev_t *indev_driver, lv_indev_data_t *data) {
    // ... existing code ...
    
    // Notify display service of activity
    if (touched) {
        settings::DisplayService::instance().markActivity();
    }
}
```

## Hardware Adaptations

### Backlight Pin

If your board uses a different pin for backlight:

**Option 1: Modify pins_config.h**
```cpp
#define LCD_LED 23  // Change to your pin
```

**Option 2: Modify settings_app.cpp**
```cpp
// In initSettingsServices():
DisplayService::instance().init(23, 0, 5000, nullptr);  // Your pin
```

### LGFX Configuration

If your LGFX setup is different, the Settings app doesn't directly use LGFX - it uses the `DisplayService` which uses ESP-IDF LEDC driver. No changes needed.

### WiFi Configuration

The WiFi service uses Arduino `WiFi.h`. If you need different WiFi settings:

```cpp
// In wifi_service.cpp, modify doConnect():
WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Adjust power if needed
```

## Memory Usage

Approximate RAM usage:
- SettingsStore: ~2 KB
- WiFiService: ~4 KB (task stack + scan results)
- TimeService: ~3 KB (task stack)
- DisplayService: ~2 KB (task + timer)
- UI Screens: ~10-15 KB (LVGL objects)
- **Total: ~25-30 KB**

## Troubleshooting

### Compilation Errors

1. **Missing fonts**: If you get font errors, change font references in screens:
   ```cpp
   // Change &lv_font_roboto_24 to available font
   &lv_font_roboto_18  // or 16, 14, etc.
   ```

2. **Include errors**: Ensure `src` is in your include path:
   ```ini
   build_flags = -Isrc
   ```

### Runtime Issues

1. **Settings not saving**: Check NVS is initialized:
   ```cpp
   #include "nvs_flash.h"
   nvs_flash_init();  // Call in setup()
   ```

2. **WiFi not connecting**: Check your board's WiFi antenna:
   ```cpp
   // In setup(), before initSettingsServices():
   WiFi.setAntenna(WIFI_ANT_INTERNAL);  // or WIFI_ANT_EXTERNAL
   ```

3. **Backlight not working**: Verify PWM pin:
   ```cpp
   // Add debug in setup():
   ESP_LOGI("MAIN", "Backlight pin: %d", LCD_LED);
   ```

## Testing Checklist

- [ ] Settings app launches from launcher
- [ ] Can navigate between screens
- [ ] Back button works correctly
- [ ] WiFi scan shows networks
- [ ] Can connect to WiFi with password
- [ ] Brightness slider changes backlight
- [ ] Timeout setting works
- [ ] Time updates correctly
- [ ] NTP sync works when WiFi connected
- [ ] Language switching works
- [ ] Settings persist after reboot
- [ ] Reset settings button works
- [ ] Reboot button works

## Advanced Customization

### Adding a New Screen

1. Create `screen_new.h` and `screen_new.cpp` in `screens/`
2. Add entry to `ScreenType` enum in `ui_router.h`
3. Add case in `UIRouter::createScreen()`
4. Add menu item in `screen_home.cpp`

### Adding New Settings

1. Add field to `SettingsData` in `settings_store.h`
2. Add getter/setter in `SettingsStore` class
3. Create UI in appropriate screen
4. Settings auto-save via `markModified()`

### Custom Themes

Modify colors in screen files:
```cpp
lv_obj_set_style_bg_color(btn, lv_color_hex(0xYOUR_COLOR), LV_PART_MAIN);
```

## Support

For issues or questions, refer to:
- `ARCHITECTURE.md` for design details
- `AGENTS.md` for project coding standards
- ESP32/Arduino forums for hardware-specific issues
