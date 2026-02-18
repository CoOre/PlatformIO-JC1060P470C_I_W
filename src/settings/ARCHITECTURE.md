# Settings App Architecture

## A) File Tree

```
src/
├── settings/                          # Settings app module
│   ├── ARCHITECTURE.md               # This file
│   │
│   ├── core/                          # Core infrastructure
│   │   ├── settings_store.h/.cpp     # In-memory settings + NVS persistence
│   │   ├── settings_persistence.h/.cpp # Debounced save, schema migration
│   │   └── settings_events.h/.cpp    # Event system for UI notifications
│   │
│   ├── services/                      # Background services (async)
│   │   ├── wifi_service.h/.cpp       # WiFi scan/connect/disconnect
│   │   ├── time_service.h/.cpp       # NTP sync, timezone, manual time
│   │   └── display_service.h/.cpp    # Brightness, backlight timeout
│   │
│   ├── i18n/                          # Internationalization
│   │   ├── i18n.h/.cpp               # Localization manager
│   │   ├── strings_ru.h              # Russian strings
│   │   └── strings_en.h              # English strings
│   │
│   ├── navigation/                    # UI navigation
│   │   ├── ui_router.h/.cpp          # Screen stack navigation
│   │   └── screen_base.h/.cpp        # Base class for all screens
│   │
│   ├── screens/                       # UI screens
│   │   ├── screen_home.h/.cpp        # Main settings menu (list)
│   │   ├── screen_wifi.h/.cpp        # WiFi settings
│   │   ├── screen_display.h/.cpp     # Display settings
│   │   ├── screen_time.h/.cpp        # Time/date settings
│   │   ├── screen_language.h/.cpp    # Language/format settings
│   │   └── screen_system.h/.cpp      # System info & controls
│   │
│   └── settings_app.h/.cpp            # Main Settings app class
│
├── ui/                                # Existing UI module
│   ├── ... (existing files)
│   └── ui_main_screen.cpp            # Modified to register Settings app
│
└── main.cpp                           # Modified to init services
```

## B) Architecture Overview

### 1. Layer Structure

```
┌─────────────────────────────────────────────────────────────┐
│  UI Layer (LVGL Task Context)                               │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐           │
│  │ Screens │ │ Screens │ │ Screens │ │ Screens │  ...      │
│  │ (Home)  │ │ (WiFi)  │ │(Display)│ │ (Time)  │           │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘           │
│       └─────────────┴──────────┴───────────┘                │
│                    │                                        │
│              ┌─────┴─────┐                                  │
│              │  Router   │  ← Navigation stack              │
│              └─────┬─────┘                                  │
│                    │                                        │
│              ┌─────┴─────┐                                  │
│              │   i18n    │  ← Localization (RU/EN)          │
│              └───────────┘                                  │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ Events (lv_event_send)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Service Layer (FreeRTOS Tasks)                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │ WiFiService │  │ TimeService │  │ DisplaySvc  │         │
│  │  (Core 0)   │  │  (Core 0)   │  │  (Core 1)   │         │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘         │
│         └────────────────┴─────────────────┘                │
│                          │                                  │
│                   ┌──────┴──────┐                           │
│                   │ Event Queue │  ← Thread-safe events      │
│                   └─────────────┘                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Persistence Layer                                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │SettingsStore│  │Persistence  │  │Preferences  │         │
│  │  (Memory)   │  │ (Debounced) │  │   (NVS)     │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│                                                               │
│  Schema version: 1                                           │
│  Auto-migration on version mismatch                          │
└─────────────────────────────────────────────────────────────┘
```

### 2. Key Design Principles

#### Non-blocking UI
- All long operations (WiFi scan, NTP sync) run in separate FreeRTOS tasks
- Services communicate with UI via `lv_event_send()` to LVGL objects
- UI updates only happen in LVGL task context

#### Debounced Persistence
- Settings changes accumulate in memory
- Auto-save after 2 seconds of inactivity
- Manual `saveNow()` for critical changes (WiFi connect)

#### Schema Versioning
```cpp
static constexpr uint32_t SETTINGS_SCHEMA_VERSION = 1;
// On boot: if stored_version != current_version → reset to defaults
```

#### Event Flow
```
WiFiService (Core 0)          UI Screen (Core 1 / LVGL task)
       │                               │
       │ scan completed                │
       ├──────────────────────────────►│
       │ (xQueueSend → lv_event_send)  │
       │                               │
       │                               │ update list
       │                               │ lv_obj_invalidate()
```

### 3. Class Responsibilities

| Class | Responsibility |
|-------|----------------|
| `SettingsStore` | Single source of truth for all settings. In-memory cache + NVS bridge |
| `SettingsPersistence` | Debounced save logic, schema migration |
| `WiFiService` | Async scan, connect, disconnect. Events: SCAN_DONE, CONNECTED, DISCONNECTED, FAILED |
| `TimeService` | NTP sync, timezone handling, manual time set. Events: TIME_SYNCED, TIME_UPDATED |
| `DisplayService` | PWM brightness control, backlight timeout timer |
| `I18n` | String lookup by key, language switching without reboot |
| `UIRouter` | Screen stack (push/pop), back navigation |
| `ScreenBase` | Common screen functionality (header, back button) |
| `SettingsApp` | Main app class, lifecycle management |

### 4. Settings Data Model

```cpp
struct SettingsData {
    // Schema
    uint32_t schema_version = 1;
    
    // WiFi
    struct {
        bool enabled = true;
        char hostname[32] = "ESP32-P4";
        struct SavedNetwork {
            char ssid[33];
            char password[65];
            bool auto_connect = true;
        } saved_networks[5];
        uint8_t saved_count = 0;
    } wifi;
    
    // Display
    struct {
        uint8_t brightness = 80;           // 0-100%
        uint32_t timeout_ms = 60000;       // 15s/30s/1m/5m/0(never)
    } display;
    
    // Time
    struct {
        char timezone[64] = "Europe/Moscow";
        bool ntp_enabled = true;
        char ntp_server[64] = "pool.ntp.org";
        int16_t manual_offset_minutes = 0; // When NTP off
    } time;
    
    // Language
    struct {
        uint8_t language = 0;              // 0=RU, 1=EN
        bool format_24h = true;
    } language;
};
```

### 5. Integration Points

The Settings app follows the existing app pattern from `BoxTestApp`:

```cpp
// In ui_main_screen.cpp createLauncher():
AppInfo settings_app;
settings_app.name = i18n::t(I18N_SETTINGS);
settings_app.icon = nullptr;
settings_app.color = 0x4CAF50;  // Green
settings_app.onLaunch = [this]() {
    launchSettingsApp();
};
launcher_->addApp(settings_app);
```

### 6. Required Hardware Abstractions

| Feature | Implementation | User Adaptation Required |
|---------|---------------|-------------------------|
| Backlight PWM | `ledcSetup()` / `ledcWrite()` | Pin number in `pins_config.h` |
| NVS | `Preferences` library | None |
| WiFi | `WiFi.h` (Arduino) | None |
| Time | `esp_sntp.h` + `setenv("TZ")` | None |

### 7. Memory Usage Estimate

| Component | RAM | Notes |
|-----------|-----|-------|
| SettingsStore | ~2 KB | Settings data + NVS buffer |
| WiFiService | ~4 KB | Task stack + scan results |
| TimeService | ~2 KB | Task stack + NTP buffers |
| DisplayService | ~1 KB | Timer + PWM state |
| UI Screens | ~10 KB | LVGL objects (in LVGL heap) |
| **Total** | **~20 KB** | Plus existing app footprint |

### 8. Thread Safety

All settings mutations go through `SettingsStore` which uses:
- `portMUX_TYPE` spinlock for critical sections
- Copy-on-read for getters
- All NVS writes happen from single Persistence task

UI updates are always in LVGL task context via `lv_event_send()`.
