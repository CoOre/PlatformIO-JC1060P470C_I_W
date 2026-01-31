#pragma once

#include "lvgl.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace ui {

/**
 * @brief Application info structure
 */
struct AppInfo {
    const char* name;
    const lv_img_dsc_t* icon;
    uint32_t color;
    std::function<void()> onLaunch;
};

/**
 * @brief App launcher - grid menu with app icons
 */
class AppLauncher {
public:
    AppLauncher();
    ~AppLauncher();

    /**
     * @brief Create the launcher on given parent
     */
    bool create(lv_obj_t* parent);

    /**
     * @brief Add an application to the launcher
     */
    void addApp(const AppInfo& app);

    /**
     * @brief Clear all apps
     */
    void clearApps();

    /**
     * @brief Set callback when app is launched
     */
    void setOnAppLaunch(std::function<void(const AppInfo&)> callback);

    /**
     * @brief Show/hide the launcher
     */
    void show();
    void hide();
    bool isVisible() const;

private:
    void createGrid();
    void onAppClicked(lv_event_t* e);
    static void appClickHandler(lv_event_t* e);

    lv_obj_t* container_ = nullptr;
    lv_obj_t* grid_ = nullptr;
    std::vector<AppInfo> apps_;
    std::function<void(const AppInfo&)> on_app_launch_;
    bool visible_ = false;

    static constexpr uint8_t kColumns = 4;
    static constexpr uint8_t kRows = 4;
    static constexpr uint8_t kIconSize = 90;
    static constexpr uint8_t kSpacing = 12;
};

} // namespace ui
