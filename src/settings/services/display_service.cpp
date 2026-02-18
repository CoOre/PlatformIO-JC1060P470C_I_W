#include "display_service.h"
#include "../core/settings_store.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include <Arduino.h>

namespace settings {

static const char* TAG = "DisplayService";
static constexpr uint32_t TASK_STACK_SIZE = 2048;
static constexpr UBaseType_t TASK_PRIORITY = 3;

// ============================================================================
// Singleton
// ============================================================================

DisplayService& DisplayService::instance() {
    static DisplayService instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool DisplayService::init(uint8_t backlight_pin, uint8_t pwm_channel,
                           uint32_t pwm_frequency, lv_obj_t* ui_event_obj) {
    if (initialized_) {
        return true;
    }
    
    backlight_pin_ = backlight_pin;
    pwm_channel_ = pwm_channel;
    pwm_frequency_ = pwm_frequency;
    ui_event_obj_ = ui_event_obj;
    
    // Load settings
    SettingsStore& store = SettingsStore::instance();
    active_brightness_ = store.getBrightness();
    idle_brightness_ = store.getIdleBrightness();
    target_brightness_ = active_brightness_;
    current_brightness_ = target_brightness_;
    timeout_ms_ = store.getDisplayTimeoutMs();
    
    // Configure LEDC
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.freq_hz = pwm_frequency_;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed: %d", err);
        return false;
    }
    
    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num = backlight_pin_;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = static_cast<ledc_channel_t>(pwm_channel_);
    channel_conf.intr_type = LEDC_INTR_DISABLE;
    channel_conf.timer_sel = LEDC_TIMER_0;
    channel_conf.duty = 0;
    channel_conf.hpoint = 0;
    
    err = ledc_channel_config(&channel_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed: %d", err);
        return false;
    }
    
    // Create mutex
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }
    
    // Create timeout timer
    if (timeout_ms_ > 0) {
        timeout_timer_ = xTimerCreate(
            "display_timeout",
            pdMS_TO_TICKS(timeout_ms_),
            pdFALSE,  // One-shot
            this,
            timeoutCallback
        );
    }
    
    // Create update task for smooth transitions
    BaseType_t ret = xTaskCreate(
        taskEntry,
        "disp_svc",
        TASK_STACK_SIZE,
        this,
        TASK_PRIORITY,
        &task_handle_
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    // Apply initial brightness
    applyBrightness(current_brightness_);
    
    // Start timeout timer if enabled
    if (timeout_timer_) {
        xTimerStart(timeout_timer_, 0);
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "Display service initialized, brightness=%d, timeout=%dms",
             target_brightness_, timeout_ms_);
    
    return true;
}

void DisplayService::deinit() {
    if (!initialized_) return;
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    
    if (timeout_timer_) {
        xTimerDelete(timeout_timer_, portMAX_DELAY);
        timeout_timer_ = nullptr;
    }
    
    if (mutex_) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
    
    // Turn off backlight
    ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(pwm_channel_), 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(pwm_channel_));
    
    initialized_ = false;
    
    ESP_LOGI(TAG, "Display service deinitialized");
}

// ============================================================================
// Task
// ============================================================================

void DisplayService::taskEntry(void* param) {
    DisplayService* self = static_cast<DisplayService*>(param);
    self->runTask();
}

void DisplayService::runTask() {
    while (true) {
        // Handle activity
        if (activity_pending_) {
            activity_pending_ = false;
            
            // Reset timer
            if (timeout_timer_ && timeout_ms_ > 0) {
                xTimerReset(timeout_timer_, 0);
            }
            
            if (dimmed_) {
                dimmed_ = false;
                target_brightness_ = active_brightness_;
            }
            
            if (current_brightness_ != target_brightness_) {
                current_brightness_ = target_brightness_;
                applyBrightness(current_brightness_);
                
                DisplayEventData evt;
                evt.event = DisplayEvent::BRIGHTNESS_CHANGED;
                evt.brightness = current_brightness_;
                notifyEvent(evt);
            }
        }
        
        // Smooth brightness transition
        if (current_brightness_ != target_brightness_) {
            updateBrightness();
        }
        
        vTaskDelay(pdMS_TO_TICKS(TRANSITION_INTERVAL_MS));
    }
}

// ============================================================================
// Timeout
// ============================================================================

void DisplayService::timeoutCallback(TimerHandle_t xTimer) {
    DisplayService* self = static_cast<DisplayService*>(pvTimerGetTimerID(xTimer));
    if (self) {
        self->onTimeout();
    }
}

void DisplayService::onTimeout() {
    ESP_LOGI(TAG, "Display timeout - dimming");
    
    // Dim first
    dimmed_ = true;
    target_brightness_ = idle_brightness_;
    
    DisplayEventData evt;
    evt.event = DisplayEvent::BACKLIGHT_DIMMED;
    notifyEvent(evt);
    
    // TODO: After another period, turn off completely
    // For now, just dim
}

// ============================================================================
// Event Handling
// ============================================================================

void DisplayService::setEventCallback(EventCallback callback) {
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        event_callback_ = callback;
        xSemaphoreGive(mutex_);
    }
}

void DisplayService::setUIEventObject(lv_obj_t* obj) {
    ui_event_obj_ = obj;
}

void DisplayService::notifyEvent(const DisplayEventData& data) {
    if (event_callback_) {
        event_callback_(data);
    }
    sendLVGLEvent(data);
}

void DisplayService::sendLVGLEvent(const DisplayEventData& data) {
    if (!ui_event_obj_) return;
    
    DisplayEventData* evt_copy = new DisplayEventData(data);
    lv_obj_send_event(ui_event_obj_, LV_EVENT_VALUE_CHANGED, evt_copy);
}

// ============================================================================
// Brightness Control
// ============================================================================

void DisplayService::setBrightness(uint8_t brightness, bool immediate) {
    brightness = brightness > 100 ? 100 : brightness;
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        active_brightness_ = brightness;
        if (!dimmed_) {
            target_brightness_ = brightness;
            if (immediate) {
                current_brightness_ = brightness;
            }
        }
        xSemaphoreGive(mutex_);
    }
    
    if (immediate && !dimmed_) {
        applyBrightness(brightness);
    }
    
    // Save to settings
    SettingsStore::instance().setBrightness(brightness);
    
    DisplayEventData evt;
    evt.event = DisplayEvent::BRIGHTNESS_CHANGED;
    evt.brightness = brightness;
    notifyEvent(evt);
    
    ESP_LOGI(TAG, "Brightness set to %d%%", brightness);
}

uint8_t DisplayService::getBrightness() const {
    return active_brightness_;
}

void DisplayService::setTargetBrightness(uint8_t brightness) {
    brightness = brightness > 100 ? 100 : brightness;
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        active_brightness_ = brightness;
        if (!dimmed_) {
            target_brightness_ = brightness;
        }
        xSemaphoreGive(mutex_);
    }
}

void DisplayService::applyBrightness(uint8_t brightness) {
    // Convert 0-100 to 0-255
    uint32_t duty = (brightness * 255) / 100;
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(pwm_channel_), duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(pwm_channel_));
}

void DisplayService::updateBrightness() {
    int diff = (int)target_brightness_ - (int)current_brightness_;
    
    if (diff > 0) {
        current_brightness_ += (diff > BRIGHTNESS_STEP) ? BRIGHTNESS_STEP : diff;
    } else if (diff < 0) {
        current_brightness_ -= (-diff > BRIGHTNESS_STEP) ? BRIGHTNESS_STEP : -diff;
    }
    
    applyBrightness(current_brightness_);
}

uint8_t DisplayService::getIdleBrightness() const {
    return idle_brightness_;
}

void DisplayService::setIdleBrightness(uint8_t brightness) {
    brightness = brightness > 100 ? 100 : brightness;
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        idle_brightness_ = brightness;
        if (dimmed_) {
            target_brightness_ = idle_brightness_;
        }
        xSemaphoreGive(mutex_);
    }
    
    SettingsStore::instance().setIdleBrightness(brightness);
}

// ============================================================================
// Timeout Control
// ============================================================================

void DisplayService::setTimeout(uint32_t timeout_ms) {
    if (timeout_ms_ == timeout_ms) return;
    
    timeout_ms_ = timeout_ms;
    
    // Save to settings
    SettingsStore::instance().setDisplayTimeoutMs(timeout_ms);
    
    // Recreate timer with new period
    if (timeout_timer_) {
        xTimerDelete(timeout_timer_, portMAX_DELAY);
        timeout_timer_ = nullptr;
    }
    
    if (timeout_ms_ > 0) {
        timeout_timer_ = xTimerCreate(
            "display_timeout",
            pdMS_TO_TICKS(timeout_ms_),
            pdFALSE,
            this,
            timeoutCallback
        );
        xTimerStart(timeout_timer_, 0);
    }
    
    DisplayEventData evt;
    evt.event = DisplayEvent::TIMEOUT_CHANGED;
    evt.timeout_ms = timeout_ms;
    notifyEvent(evt);
    
    ESP_LOGI(TAG, "Timeout set to %dms", timeout_ms);
}

uint32_t DisplayService::getTimeout() const {
    return timeout_ms_;
}

const char* DisplayService::getTimeoutString() const {
    static char buf[16];
    
    switch (timeout_ms_) {
        case 0:
            strcpy(buf, "Never");
            break;
        case 15000:
            strcpy(buf, "15s");
            break;
        case 30000:
            strcpy(buf, "30s");
            break;
        case 60000:
            strcpy(buf, "1m");
            break;
        case 300000:
            strcpy(buf, "5m");
            break;
        default:
            snprintf(buf, sizeof(buf), "%ds", timeout_ms_ / 1000);
            break;
    }
    
    return buf;
}

// ============================================================================
// Activity Tracking
// ============================================================================

void DisplayService::markActivity() {
    activity_pending_ = true;
}

void DisplayService::wakeUp() {
    markActivity();
}

void DisplayService::forceOff() {
    target_brightness_ = 0;
    current_brightness_ = 0;
    applyBrightness(0);
    
    if (timeout_timer_) {
        xTimerStop(timeout_timer_, 0);
    }
}

// ============================================================================
// Rotation Control
// ============================================================================

void DisplayService::setRotation(uint8_t rotation, void* lcd_ptr, lv_display_t* disp_ptr) {
    // Clamp to valid range
    if (rotation > 3) rotation = 0;
    
    rotation_ = rotation;
    
    // Save to settings
    SettingsStore::instance().setRotation(rotation);
    
    // Apply LVGL rotation if display pointer provided
    if (disp_ptr) {
        lv_display_set_rotation(disp_ptr, static_cast<lv_display_rotation_t>(rotation));
    }
    
    // Apply hardware rotation if LCD pointer provided
    // Note: This requires the LGFX instance, which should be passed from main.cpp
    // The actual hardware rotation is handled in main.cpp via a callback
    
    ESP_LOGI(TAG, "Rotation set to %d (%d\xc2\xb0)", rotation, rotation * 90);
}

uint8_t DisplayService::getRotation() const {
    return rotation_;
}

} // namespace settings
