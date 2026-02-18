#pragma once

#include "lvgl.h"
#include "settings_store.h"

namespace settings {

struct ThemeColors {
    lv_color_t screen_bg;
    lv_color_t card_bg;
    lv_color_t card_border;
    lv_color_t title;
    lv_color_t muted;
    lv_color_t accent;
    lv_color_t accent_soft;
    lv_color_t status_bg;
    lv_color_t status_fg;
    lv_color_t overlay;
    lv_opa_t overlay_opa;
};

ThemeColors getThemeColors(ThemeMode mode);
ThemeColors currentThemeColors();

} // namespace settings
