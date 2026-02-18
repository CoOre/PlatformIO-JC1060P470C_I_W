#include "theme.h"
#include "settings_store.h"

namespace settings {

ThemeColors getThemeColors(ThemeMode mode) {
    ThemeColors colors{};
    if (mode == ThemeMode::DARK) {
        colors.screen_bg = lv_color_hex(0x0F1116);
        colors.card_bg = lv_color_hex(0x171A20);
        colors.card_border = lv_color_hex(0x262C36);
        colors.title = lv_color_hex(0xE7EAF0);
        colors.muted = lv_color_hex(0xA3A9B6);
        colors.accent = lv_color_hex(0x606A7C);
        colors.accent_soft = lv_color_hex(0x3A3F4B);
        colors.status_bg = lv_color_hex(0x171A20);
        colors.status_fg = lv_color_hex(0xE7EAF0);
        colors.overlay = lv_color_hex(0x000000);
        colors.overlay_opa = LV_OPA_70;
    } else {
        colors.screen_bg = lv_color_hex(0xF2F3F5);
        colors.card_bg = lv_color_hex(0xFFFFFF);
        colors.card_border = lv_color_hex(0xD9DDE4);
        colors.title = lv_color_hex(0x1E2026);
        colors.muted = lv_color_hex(0x6C717C);
        colors.accent = lv_color_hex(0x3A3F4B);
        colors.accent_soft = lv_color_hex(0xB7BDC7);
        colors.status_bg = lv_color_hex(0xFFFFFF);
        colors.status_fg = lv_color_hex(0x1E2026);
        colors.overlay = lv_color_hex(0x000000);
        colors.overlay_opa = LV_OPA_40;
    }
    return colors;
}

ThemeColors currentThemeColors() {
    ThemeMode mode = SettingsStore::instance().getTheme();
    return getThemeColors(mode);
}

} // namespace settings
