#include "app_launcher.h"
#include "ui_manager.h"

namespace ui {

AppLauncher::AppLauncher() = default;

AppLauncher::~AppLauncher() {
    if (container_) {
        lv_obj_del(container_);
    }
}

bool AppLauncher::create(lv_obj_t* parent) {
    if (!parent) return false;

    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(container_, LV_OBJ_FLAG_SCROLLABLE);

    // White background
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container_, 16, LV_PART_MAIN);
    // Use full width; keep a bit of vertical air.
    lv_obj_set_style_pad_hor(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);

    createGrid();
    visible_ = true;
    return true;
}

void AppLauncher::createGrid() {
    grid_ = lv_obj_create(container_);
    lv_obj_set_size(grid_, LV_PCT(100), LV_PCT(100));
    lv_obj_clear_flag(grid_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(grid_, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(grid_, 0, LV_PART_MAIN);
    
    // Grid layout - 4x4 with equal column and row sizes
    lv_obj_set_layout(grid_, LV_LAYOUT_GRID);
    // Grid templates must outlive the object; keep them static.
    static const lv_coord_t kGridCols[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    static const lv_coord_t kGridRows[] = {
        LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST
    };
    lv_obj_set_grid_dsc_array(grid_,
        kGridCols,
        kGridRows);
    
    lv_obj_set_style_pad_row(grid_, kSpacing, LV_PART_MAIN);
    lv_obj_set_style_pad_column(grid_, kSpacing, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(grid_, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid_, 0, LV_PART_MAIN);
}

void AppLauncher::addApp(const AppInfo& app) {
    apps_.push_back(app);
    
    uint8_t col = (apps_.size() - 1) % kColumns;
    uint8_t row = (apps_.size() - 1) / kColumns;

    // Container for icon + label (fills the grid cell)
    lv_obj_t* container = lv_obj_create(grid_);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_grid_cell(container, LV_GRID_ALIGN_STRETCH, col, 1, LV_GRID_ALIGN_STRETCH, row, 1);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(container, 4, LV_PART_MAIN);

    // App icon - square with rounded corners
    lv_obj_t* btn = lv_obj_create(container);
    lv_obj_set_size(btn, kIconSize, kIconSize);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    
    // Style - square with rounded corners
    lv_obj_set_style_bg_color(btn, lv_color_hex(app.color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 16, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 4, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(btn, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_20, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);

    // App name label - below the icon
    lv_obj_t* label = lv_label_create(container);
    lv_label_set_text(label, app.name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0x333333), LV_PART_MAIN);

    // Store app index in user data (on the icon button)
    size_t index = apps_.size() - 1;
    lv_obj_set_user_data(btn, reinterpret_cast<void*>(index));
    lv_obj_add_event_cb(btn, appClickHandler, LV_EVENT_CLICKED, this);
}

void AppLauncher::clearApps() {
    apps_.clear();
    if (grid_) {
        lv_obj_clean(grid_);
    }
}

void AppLauncher::setOnAppLaunch(std::function<void(const AppInfo&)> callback) {
    on_app_launch_ = callback;
}

void AppLauncher::show() {
    if (container_) {
        lv_obj_clear_flag(container_, LV_OBJ_FLAG_HIDDEN);
        visible_ = true;
    }
}

void AppLauncher::hide() {
    if (container_) {
        lv_obj_add_flag(container_, LV_OBJ_FLAG_HIDDEN);
        visible_ = false;
    }
}

bool AppLauncher::isVisible() const {
    return visible_;
}

void AppLauncher::appClickHandler(lv_event_t* e) {
    auto* launcher = static_cast<AppLauncher*>(lv_event_get_user_data(e));
    launcher->onAppClicked(e);
}

void AppLauncher::onAppClicked(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target(e));
    size_t index = reinterpret_cast<size_t>(lv_obj_get_user_data(btn));
    
    if (index < apps_.size() && on_app_launch_) {
        on_app_launch_(apps_[index]);
    }
}

} // namespace ui
