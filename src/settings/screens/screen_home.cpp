#include "screen_home.h"
#include "../i18n/i18n.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "HomeScreen";

// Menu item data - defined here for internal use
struct MenuItem {
    StringID label;
    ScreenType screen;
    const char* icon;  // UTF-8 icon or nullptr
};

static const MenuItem MENU_ITEMS[] = {
    { StringID::SETTINGS_WIFI,     ScreenType::WIFI,     "\xEF\x87\xAB" },  // WiFi icon
    { StringID::SETTINGS_DISPLAY,  ScreenType::DISPLAY,  "\xEF\x84\x88" },  // Sun icon
    { StringID::SETTINGS_TIME,     ScreenType::TIME,     "\xEF\x80\x97" },  // Clock icon
    { StringID::SETTINGS_LANGUAGE, ScreenType::LANGUAGE, "\xEF\x86\xAB" },  // Globe icon
    { StringID::SETTINGS_SYSTEM,   ScreenType::SYSTEM,   "\xEF\x85\x9B" },  // Cog icon
};

static const uint32_t MENU_ITEM_HEIGHT = 80;  // Увеличенная высота пунктов
static const uint32_t HEADER_HEIGHT = 64;
static const uint32_t ICON_SIZE = 40;

// ============================================================================
// Lifecycle
// ============================================================================

bool HomeScreen::create(lv_obj_t* parent) {
    if (!parent) return false;

    setupContainer(parent);
    theme_ = currentThemeColors();
    lv_obj_set_style_bg_color(container_, theme_.screen_bg, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);

    createHeader();
    createMenu();

    ESP_LOGI(TAG, "Home screen created");
    return true;
}

void HomeScreen::destroy() {
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        header_ = nullptr;
        menu_list_ = nullptr;
    }

    ESP_LOGI(TAG, "Home screen destroyed");
}

void HomeScreen::onShow() {
    // Refresh header title in case language changed
    if (header_) {
        lv_obj_t* title = lv_obj_get_child(header_, 0);
        if (title) {
            lv_label_set_text(title, S(SETTINGS_TITLE));
        }
    }
}

// ============================================================================
// UI Creation
// ============================================================================

void HomeScreen::createHeader() {
    // Header container
    header_ = lv_obj_create(container_);
    lv_obj_set_size(header_, LV_PCT(100), HEADER_HEIGHT);
    lv_obj_set_style_pad_all(header_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(header_, 24, LV_PART_MAIN);
    lv_obj_set_style_pad_right(header_, 24, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(header_, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_radius(header_, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(header_, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Title
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(SETTINGS_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, theme_.title, LV_PART_MAIN);
}

void HomeScreen::createMenu() {
    // Menu container (не используем lv_list, делаем свой flex layout)
    menu_list_ = lv_obj_create(container_);
    lv_obj_set_size(menu_list_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(menu_list_, 16, LV_PART_MAIN);
    lv_obj_set_style_pad_top(menu_list_, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(menu_list_, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(menu_list_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(menu_list_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_flex_flow(menu_list_, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(menu_list_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(menu_list_, 8, LV_PART_MAIN);

    // Disable scroll on container - items should fit
    lv_obj_set_scrollbar_mode(menu_list_, LV_SCROLLBAR_MODE_OFF);

    // Add menu items
    for (const auto& item : MENU_ITEMS) {
        lv_obj_t* btn = createMenuItem(menu_list_, item);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(item.screen));
        lv_obj_add_event_cb(btn, onMenuItemClicked, LV_EVENT_CLICKED, this);
    }
}

lv_obj_t* HomeScreen::createMenuItem(lv_obj_t* parent, const MenuItem& item) {
    // Button container
    lv_obj_t* btn = lv_obj_create(parent);
    lv_obj_set_size(btn, LV_PCT(100), MENU_ITEM_HEIGHT);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, 20, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, theme_.card_bg, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Icon container (rounded background)
    lv_obj_t* icon_bg = lv_obj_create(btn);
    lv_obj_set_size(icon_bg, ICON_SIZE, ICON_SIZE);
    lv_obj_set_style_bg_color(icon_bg, theme_.accent_soft, LV_PART_MAIN);
    lv_obj_set_style_radius(icon_bg, ICON_SIZE / 2, LV_PART_MAIN);
    lv_obj_set_style_border_width(icon_bg, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(icon_bg, 0, LV_PART_MAIN);

    // Icon
    lv_obj_t* icon_label = lv_label_create(icon_bg);
    lv_label_set_text(icon_label, item.icon);
    lv_obj_set_style_text_font(icon_label, &lv_font_roboto_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(icon_label, theme_.title, LV_PART_MAIN);
    lv_obj_center(icon_label);

    // Text label
    lv_obj_t* text_label = lv_label_create(btn);
    lv_label_set_text(text_label, t(item.label));
    lv_obj_set_style_text_font(text_label, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_set_style_text_color(text_label, theme_.title, LV_PART_MAIN);
    lv_obj_set_style_pad_left(text_label, 16, LV_PART_MAIN);

    // Chevron (arrow right)
    lv_obj_t* chevron = lv_label_create(btn);
    lv_label_set_text(chevron, "\xEF\x81\x94");  // fa-chevron-right
    lv_obj_set_style_text_font(chevron, &lv_font_roboto_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(chevron, theme_.muted, LV_PART_MAIN);
    lv_obj_set_flex_grow(chevron, 1);
    lv_obj_set_style_text_align(chevron, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);

    // Pressed state
    lv_obj_set_style_bg_color(btn, theme_.accent_soft, LV_STATE_PRESSED);

    return btn;
}

// ============================================================================
// Event Handlers
// ============================================================================

void HomeScreen::onMenuItemClicked(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    ScreenType screen_type = static_cast<ScreenType>(
        reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn))
    );
    
    ESP_LOGI(TAG, "Menu item clicked, navigating to screen %d", static_cast<int>(screen_type));
    
    UIRouter::instance().push(screen_type);
}

} // namespace settings
