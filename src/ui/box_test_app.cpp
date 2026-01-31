#include "box_test_app.h"
#include "ui_manager.h"

namespace ui {

BoxTestApp::BoxTestApp() = default;

BoxTestApp::~BoxTestApp() {
    destroy();
}

bool BoxTestApp::create(lv_obj_t* parent) {
    if (!parent) return false;
    if (container_) return true; // Already created

    setupContainer(parent);
    createBoxes();
    setupAutoScroll();

    return true;
}

void BoxTestApp::destroy() {
    if (auto_scroll_timer_) {
        lv_timer_delete(auto_scroll_timer_);
        auto_scroll_timer_ = nullptr;
    }
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
        content_ = nullptr;
    }
}

void BoxTestApp::setupContainer(lv_obj_t* parent) {
    // Main container fills entire parent and is scrollable
    container_ = lv_obj_create(parent);
    lv_obj_set_size(container_, LV_PCT(100), LV_PCT(100));

    // Remove padding
    lv_obj_set_style_pad_all(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_ver(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_margin_all(container_, 0, LV_PART_MAIN);

    // Style
    lv_obj_set_style_bg_color(container_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(container_, LV_OPA_100, LV_PART_MAIN);
    lv_obj_set_style_radius(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(container_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(container_, 0, LV_PART_MAIN);

    // Enable scrolling - container itself scrolls
    lv_obj_set_scroll_dir(container_, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container_, LV_SCROLLBAR_MODE_AUTO);
}

void BoxTestApp::createBoxes() {
    // Content container inside scrollable area
    content_ = lv_obj_create(container_);
    lv_obj_set_width(content_, LV_PCT(100));
    lv_obj_set_height(content_, 1000); // Fixed height for scrolling
    lv_obj_clear_flag(content_, LV_OBJ_FLAG_SCROLLABLE);

    // Transparent style
    lv_obj_set_style_pad_all(content_, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(content_, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(content_, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, LV_PART_MAIN);

    // Screen dimensions - use actual screen width from UIManager
    const lv_coord_t max_w = UIManager::screenWidth();
    const lv_coord_t max_h = 1000;

    // Create test boxes
    for (uint32_t i = 0; i < kBoxCount; ++i) {
        const lv_coord_t w = 40 + (i * 13u % 120);
        const lv_coord_t h = 30 + (i * 17u % 100);

        // X coordinate - spread across full width
        const lv_coord_t x = (i * 37u) % ((max_w > w) ? (max_w - w) : 1);

        // Y coordinate
        const lv_coord_t y = (i * 53u) % ((max_h > h) ? (max_h - h) : 1);

        lv_obj_t* box = lv_obj_create(content_);
        lv_obj_set_size(box, w, h);
        lv_obj_set_pos(box, x, y);

        // Style
        lv_obj_set_style_radius(box, 6, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);

        // Random color
        const uint32_t color = ((i * 97u) % 255) << 16 |
                               ((i * 57u) % 255) << 8 |
                               ((i * 23u) % 255);
        lv_obj_set_style_bg_color(box, lv_color_hex(color), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);

        // Label
        lv_obj_t* label = lv_label_create(box);
        lv_label_set_text_fmt(label, "#%02u", static_cast<unsigned>(i));
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    }
}

void BoxTestApp::setupAutoScroll() {
    auto_scroll_timer_ = lv_timer_create([](lv_timer_t* t) {
        // Placeholder for auto-scroll logic
    }, 16, nullptr);
}

void BoxTestApp::update(uint32_t now_ms) {
    (void)now_ms;

    if (auto_scroll_timer_) {
        bool touch_active = UIManager::instance().isTouchActive();

        if (touch_active && !auto_scroll_paused_) {
            lv_timer_pause(auto_scroll_timer_);
            auto_scroll_paused_ = true;
        } else if (!touch_active && auto_scroll_paused_) {
            lv_timer_resume(auto_scroll_timer_);
            auto_scroll_paused_ = false;
        }
    }
}

void BoxTestApp::setAutoScrollPaused(bool paused) {
    if (!auto_scroll_timer_) return;

    if (paused && !auto_scroll_paused_) {
        lv_timer_pause(auto_scroll_timer_);
        auto_scroll_paused_ = true;
    } else if (!paused && auto_scroll_paused_) {
        lv_timer_resume(auto_scroll_timer_);
        auto_scroll_paused_ = false;
    }
}

} // namespace ui
