#pragma once

#include "lvgl.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace settings {

// ============================================================================
// Screen Types
// ============================================================================

enum class ScreenType : uint8_t {
    NONE,
    HOME,
    WIFI,
    DISPLAY,
    TIME,
    LANGUAGE,
    SYSTEM
};

// ============================================================================
// Screen Base Class
// ============================================================================

/**
 * @brief Base class for all settings screens
 * 
 * Each screen manages its own LVGL objects and lifecycle.
 * Screens are created on navigation and destroyed on back/exit.
 */
class Screen {
public:
    Screen() = default;
    virtual ~Screen() = default;
    
    // Disable copy/move
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;
    
    /**
     * @brief Create the screen on given parent
     * @param parent LVGL parent object (usually a container filling the screen)
     * @return true if successful
     */
    virtual bool create(lv_obj_t* parent) = 0;
    
    /**
     * @brief Destroy the screen and all its LVGL objects
     */
    virtual void destroy() = 0;
    
    /**
     * @brief Update screen (called periodically from main loop)
     * @param now_ms current time in milliseconds
     */
    virtual void update(uint32_t now_ms) {}
    
    /**
     * @brief Called when screen becomes active (pushed to stack)
     */
    virtual void onShow() {}
    
    /**
     * @brief Called when screen becomes inactive (popped from stack)
     */
    virtual void onHide() {}
    
    /**
     * @brief Handle back button press
     * @return true if handled, false if router should pop
     */
    virtual bool onBack() { return false; }
    
    /**
     * @brief Check if screen is currently created
     */
    bool isCreated() const { return container_ != nullptr; }
    
    /**
     * @brief Get screen type
     */
    virtual ScreenType getType() const = 0;
    
    /**
     * @brief Get container object
     */
    lv_obj_t* getContainer() const { return container_; }

protected:
    lv_obj_t* container_ = nullptr;
    
    /**
     * @brief Setup common container styles
     */
    void setupContainer(lv_obj_t* parent);
};

// ============================================================================
// Router
// ============================================================================

/**
 * @brief Screen stack router for settings app navigation
 * 
 * Manages a stack of screens with push/pop navigation.
 * Only the top screen is visible and receiving updates.
 */
class UIRouter {
public:
    static UIRouter& instance();
    
    // Disable copy/move
    UIRouter(const UIRouter&) = delete;
    UIRouter& operator=(const UIRouter&) = delete;
    
    /**
     * @brief Initialize router
     * @param parent Parent LVGL object for all screens
     * @return true if successful
     */
    bool init(lv_obj_t* parent);
    
    /**
     * @brief Deinitialize and destroy all screens
     */
    void deinit();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return parent_ != nullptr; }
    
    // ============================================================================
    // Navigation
    // ============================================================================
    
    /**
     * @brief Push a new screen onto the stack
     * @param screen Screen to push (router takes ownership)
     * @return true if successful
     */
    bool push(Screen* screen);
    
    /**
     * @brief Push a screen by type (creates new instance)
     */
    bool push(ScreenType type);
    
    /**
     * @brief Pop current screen from stack
     * @return true if there was a screen to pop
     */
    bool pop();
    
    /**
     * @brief Pop to root (home) screen
     */
    void popToRoot();
    
    /**
     * @brief Replace current screen with new one
     */
    bool replace(Screen* screen);
    
    /**
     * @brief Handle back button
     * @return true if handled (popped or screen handled it)
     */
    bool goBack();

    /**
     * @brief Recreate all screens in stack to apply theme changes
     */
    void reloadTheme();
    
    // ============================================================================
    // Queries
    // ============================================================================
    
    /**
     * @brief Get current (top) screen
     */
    Screen* getCurrentScreen() const;
    
    /**
     * @brief Get current screen type
     */
    ScreenType getCurrentType() const;
    
    /**
     * @brief Get stack depth
     */
    size_t getStackDepth() const { return stack_.size(); }
    
    /**
     * @brief Check if can go back
     */
    bool canGoBack() const { return stack_.size() > 1; }
    
    // ============================================================================
    // Update
    // ============================================================================
    
    /**
     * @brief Update current screen
     */
    void update(uint32_t now_ms);

private:
    UIRouter() = default;
    ~UIRouter() = default;
    
    lv_obj_t* parent_ = nullptr;
    std::vector<Screen*> stack_;
};

// ============================================================================
// Screen Factory
// ============================================================================

/**
 * @brief Create a screen by type
 */
Screen* createScreen(ScreenType type);

} // namespace settings
