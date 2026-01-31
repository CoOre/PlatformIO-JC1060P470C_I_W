#pragma once

#include "lvgl.h"
#include <cstdint>

namespace ui {

/**
 * @brief Box test application - FPS test with colored boxes
 * 
 * This was originally the main screen content, now moved to a separate "app"
 */
class BoxTestApp {
public:
    BoxTestApp();
    ~BoxTestApp();

    /**
     * @brief Create the app on given parent
     */
    bool create(lv_obj_t* parent);

    /**
     * @brief Destroy the app and free resources
     */
    void destroy();

    /**
     * @brief Update app logic (call from main loop)
     */
    void update(uint32_t now_ms);

    /**
     * @brief Set auto-scroll pause state
     */
    void setAutoScrollPaused(bool paused);

    /**
     * @brief Check if app is currently created
     */
    bool isActive() const { return container_ != nullptr; }

private:
    void setupContainer(lv_obj_t* parent);
    void createBoxes();
    void setupAutoScroll();

    lv_obj_t* container_ = nullptr;
    lv_obj_t* content_ = nullptr;
    lv_timer_t* auto_scroll_timer_ = nullptr;
    
    bool auto_scroll_paused_ = false;
    static constexpr uint32_t kBoxCount = 80;
};

} // namespace ui
