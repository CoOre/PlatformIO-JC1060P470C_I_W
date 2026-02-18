#pragma once

#include "../navigation/ui_router.h"

namespace settings {

// Forward declaration for menu item
struct MenuItem;

/**
 * @brief Settings home screen - main menu
 *
 * Shows list of settings categories:
 * - Wi-Fi
 * - Display
 * - Time
 * - Language
 * - System
 */
class HomeScreen : public Screen {
public:
    HomeScreen() = default;
    ~HomeScreen() override = default;

    bool create(lv_obj_t* parent) override;
    void destroy() override;
    void onShow() override;
    ScreenType getType() const override { return ScreenType::HOME; }

private:
    void createHeader();
    void createMenu();
    lv_obj_t* createMenuItem(lv_obj_t* parent, const MenuItem& item);

    static void onMenuItemClicked(lv_event_t* e);

    lv_obj_t* header_ = nullptr;
    lv_obj_t* menu_list_ = nullptr;
};

} // namespace settings
