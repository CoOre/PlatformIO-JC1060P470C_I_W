#include "settings_persistence.h"
#include "settings_store.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "SettingsPersistence";
static constexpr uint32_t SAVE_TASK_STACK_SIZE = 4096;
static constexpr UBaseType_t SAVE_TASK_PRIORITY = 3;
static constexpr uint32_t SAVE_QUEUE_SIZE = 4;

// ============================================================================
// Singleton
// ============================================================================

SettingsPersistence& SettingsPersistence::instance() {
    static SettingsPersistence instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool SettingsPersistence::init(uint32_t debounce_ms) {
    if (timer_) {
        return true;  // Already initialized
    }
    
    debounce_ms_ = debounce_ms;
    
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }
    
    // Create queue for save commands
    save_queue_ = xQueueCreate(SAVE_QUEUE_SIZE, sizeof(SaveCommand));
    if (!save_queue_) {
        ESP_LOGE(TAG, "Failed to create save queue");
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    // Create save task with larger stack
    BaseType_t ret = xTaskCreate(
        saveTaskEntry,
        "settings_save",
        SAVE_TASK_STACK_SIZE,
        this,
        SAVE_TASK_PRIORITY,
        &save_task_
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create save task");
        vQueueDelete(save_queue_);
        save_queue_ = nullptr;
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    // Create timer for debouncing (only signals, doesn't do actual save)
    timer_ = xTimerCreate(
        "settings_timer",
        pdMS_TO_TICKS(debounce_ms_),
        pdFALSE,  // One-shot
        this,
        timerCallback
    );
    
    if (!timer_) {
        ESP_LOGE(TAG, "Failed to create timer");
        // Signal task to shutdown
        SaveCommand cmd = SaveCommand::SHUTDOWN;
        xQueueSend(save_queue_, &cmd, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100));
        vTaskDelete(save_task_);
        save_task_ = nullptr;
        vQueueDelete(save_queue_);
        save_queue_ = nullptr;
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    task_running_ = true;
    ESP_LOGI(TAG, "Initialized with debounce=%dms, save task stack=%d",
             debounce_ms_, SAVE_TASK_STACK_SIZE);
    return true;
}

void SettingsPersistence::deinit() {
    // Stop timer
    if (timer_) {
        xTimerDelete(timer_, portMAX_DELAY);
        timer_ = nullptr;
    }
    
    // Signal save task to shutdown
    if (save_queue_ && save_task_) {
        SaveCommand cmd = SaveCommand::SHUTDOWN;
        xQueueSend(save_queue_, &cmd, portMAX_DELAY);
        
        // Wait for task to finish
        vTaskDelay(pdMS_TO_TICKS(100));
        
        vTaskDelete(save_task_);
        save_task_ = nullptr;
        
        vQueueDelete(save_queue_);
        save_queue_ = nullptr;
    }
    
    if (mutex_) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
    
    task_running_ = false;
    save_pending_ = false;
    
    ESP_LOGI(TAG, "Deinitialized");
}

// ============================================================================
// Save Task
// ============================================================================

void SettingsPersistence::saveTaskEntry(void* param) {
    SettingsPersistence* self = static_cast<SettingsPersistence*>(param);
    self->runSaveTask();
}

void SettingsPersistence::runSaveTask() {
    SaveCommand cmd;
    
    while (true) {
        // Wait for save command
        if (xQueueReceive(save_queue_, &cmd, portMAX_DELAY) == pdTRUE) {
            switch (cmd) {
                case SaveCommand::SAVE:
                    ESP_LOGI(TAG, "Save task performing save...");
                    SettingsStore::instance().saveNow();
                    ESP_LOGI(TAG, "Save task completed save");
                    break;
                    
                case SaveCommand::SHUTDOWN:
                    ESP_LOGI(TAG, "Save task shutting down");
                    return;
            }
        }
    }
}

void SettingsPersistence::triggerSave() {
    if (!save_queue_) return;
    
    SaveCommand cmd = SaveCommand::SAVE;
    // Non-blocking send - if queue is full, another save is pending
    xQueueSend(save_queue_, &cmd, 0);
}

// ============================================================================
// Timer Callback
// ============================================================================

void SettingsPersistence::timerCallback(TimerHandle_t xTimer) {
    SettingsPersistence* self = static_cast<SettingsPersistence*>(pvTimerGetTimerID(xTimer));
    if (self) {
        self->onTimer();
    }
}

void SettingsPersistence::onTimer() {
    ESP_LOGI(TAG, "Auto-save timer fired, signaling save task");
    
    // Signal save task instead of doing save directly
    // This avoids stack overflow in timer task
    triggerSave();
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        save_pending_ = false;
        xSemaphoreGive(mutex_);
    }
}

// ============================================================================
// Public API
// ============================================================================

void SettingsPersistence::markModified() {
    if (!timer_ || !task_running_) {
        ESP_LOGW(TAG, "Not initialized, saving immediately in current context");
        SettingsStore::instance().saveNow();
        return;
    }
    
    // Reset the timer (debounce)
    xTimerStop(timer_, portMAX_DELAY);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        save_pending_ = true;
        xSemaphoreGive(mutex_);
    }
    
    xTimerChangePeriod(timer_, pdMS_TO_TICKS(debounce_ms_), portMAX_DELAY);
    xTimerStart(timer_, portMAX_DELAY);
    
    ESP_LOGD(TAG, "Save scheduled in %dms", debounce_ms_);
}

void SettingsPersistence::saveNow() {
    if (!timer_ || !task_running_) {
        SettingsStore::instance().saveNow();
        return;
    }
    
    // Stop any pending timer
    xTimerStop(timer_, portMAX_DELAY);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        save_pending_ = false;
        xSemaphoreGive(mutex_);
    }
    
    // Signal save task for immediate save
    ESP_LOGI(TAG, "Immediate save requested, signaling save task");
    triggerSave();
}

bool SettingsPersistence::isSavePending() const {
    bool pending = false;
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        pending = save_pending_;
        xSemaphoreGive(mutex_);
    }
    return pending;
}

uint32_t SettingsPersistence::getTimeUntilSave() const {
    if (!timer_ || !isSavePending()) {
        return 0;
    }
    
    // FreeRTOS doesn't provide direct time remaining API
    // Return debounce time as approximation
    return debounce_ms_;
}

void SettingsPersistence::setDebounceMs(uint32_t debounce_ms) {
    debounce_ms_ = debounce_ms;
    
    // If timer is active, restart with new period
    if (timer_ && isSavePending()) {
        xTimerChangePeriod(timer_, pdMS_TO_TICKS(debounce_ms_), portMAX_DELAY);
    }
}

// ============================================================================
// Batch Guard
// ============================================================================

SettingsBatchGuard::SettingsBatchGuard() {
    // Increase debounce time during batch operations
    // This is a no-op for now, but could be used for optimization
}

SettingsBatchGuard::~SettingsBatchGuard() {
    if (!committed_) {
        SettingsPersistence::instance().markModified();
    }
}

void SettingsBatchGuard::commitNow() {
    committed_ = true;
    SettingsPersistence::instance().saveNow();
}

} // namespace settings
