#include "wifi_service.h"
#include "../core/settings_store.h"
#include <WiFi.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "lvgl.h"

namespace settings {

static const char* TAG = "WiFiService";
static constexpr uint32_t TASK_STACK_SIZE = 4096;
static constexpr UBaseType_t TASK_PRIORITY = 5;
static constexpr uint32_t CMD_QUEUE_SIZE = 10;

// ============================================================================
// Singleton
// ============================================================================

WiFiService& WiFiService::instance() {
    static WiFiService instance;
    return instance;
}

// ============================================================================
// Initialization
// ============================================================================

bool WiFiService::init(lv_obj_t* ui_event_obj) {
    if (initialized_) {
        return true;
    }
    
    ui_event_obj_ = ui_event_obj;
    
    // Create mutex
    mutex_ = xSemaphoreCreateMutex();
    if (!mutex_) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return false;
    }
    
    // Create command queue
    cmd_queue_ = xQueueCreate(CMD_QUEUE_SIZE, sizeof(Command));
    if (!cmd_queue_) {
        ESP_LOGE(TAG, "Failed to create command queue");
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
        return false;
    }
    
    // Initialize WiFi - add delay for ESP32-P4 hosted WiFi to be ready
    vTaskDelay(pdMS_TO_TICKS(500));
    
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);  // We handle this ourselves
    
    // Set hostname
    SettingsStore& store = SettingsStore::instance();
    WiFi.setHostname(store.getHostname());
    
    ESP_LOGI(TAG, "WiFi mode set to STA, hostname: %s", store.getHostname());
    
    // Create task
    BaseType_t ret = xTaskCreate(
        taskEntry,
        "wifi_svc",
        TASK_STACK_SIZE,
        this,
        TASK_PRIORITY,
        &task_handle_
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        vQueueDelete(cmd_queue_);
        vSemaphoreDelete(mutex_);
        cmd_queue_ = nullptr;
        mutex_ = nullptr;
        return false;
    }
    
    initialized_ = true;
    ESP_LOGI(TAG, "WiFi service initialized");
    
    // Auto-connect if enabled - delay to let WiFi stack fully initialize
    if (store.getWiFiEnabled()) {
        vTaskDelay(pdMS_TO_TICKS(1000));  // Give WiFi hardware time to be ready
        autoConnect();
    }
    
    return true;
}

void WiFiService::deinit() {
    if (!initialized_) return;
    
    // Send stop command
    Command cmd;
    cmd.type = CommandType::STOP;
    xQueueSend(cmd_queue_, &cmd, portMAX_DELAY);
    
    // Wait for task to finish
    vTaskDelay(pdMS_TO_TICKS(100));
    
    if (task_handle_) {
        vTaskDelete(task_handle_);
        task_handle_ = nullptr;
    }
    
    if (cmd_queue_) {
        vQueueDelete(cmd_queue_);
        cmd_queue_ = nullptr;
    }
    
    if (mutex_) {
        vSemaphoreDelete(mutex_);
        mutex_ = nullptr;
    }
    
    WiFi.mode(WIFI_OFF);
    initialized_ = false;
    
    ESP_LOGI(TAG, "WiFi service deinitialized");
}

// ============================================================================
// Task
// ============================================================================

void WiFiService::taskEntry(void* param) {
    WiFiService* self = static_cast<WiFiService*>(param);
    self->runTask();
}

void WiFiService::runTask() {
    Command cmd;
    
    while (true) {
        if (xQueueReceive(cmd_queue_, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (cmd.type == CommandType::STOP) {
                break;
            }
            processCommand(cmd);
        }
        
        // Update RSSI periodically if connected
        static uint32_t last_rssi_check = 0;
        uint32_t now = millis();
        if (now - last_rssi_check > 5000) {  // Every 5 seconds
            last_rssi_check = now;
            if (WiFi.status() == WL_CONNECTED) {
                int8_t new_rssi = WiFi.RSSI();
                if (mutex_) {
                    xSemaphoreTake(mutex_, portMAX_DELAY);
                    if (rssi_ != new_rssi) {
                        rssi_ = new_rssi;
                        xSemaphoreGive(mutex_);
                        
                        WiFiEventData evt;
                        evt.event = WiFiEvent::RSSI_UPDATED;
                        strncpy(evt.ssid, connected_ssid_, sizeof(evt.ssid));
                        notifyEvent(evt);
                    } else {
                        xSemaphoreGive(mutex_);
                    }
                }
            }
        }
    }
    
    vTaskDelete(nullptr);
}

void WiFiService::processCommand(const Command& cmd) {
    switch (cmd.type) {
        case CommandType::SCAN:
            doScan();
            break;
        case CommandType::CONNECT:
            doConnect(cmd.ssid, cmd.password, cmd.save);
            break;
        case CommandType::DISCONNECT:
            doDisconnect();
            break;
        case CommandType::AUTO_CONNECT:
            doAutoConnect();
            break;
        case CommandType::FORGET:
            // Handled in main thread
            break;
        case CommandType::STOP:
            break;
    }
}

// ============================================================================
// Event Handling
// ============================================================================

void WiFiService::setEventCallback(EventCallback callback) {
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        event_callback_ = callback;
        xSemaphoreGive(mutex_);
    }
}

void WiFiService::setUIEventObject(lv_obj_t* obj) {
    ui_event_obj_ = obj;
}

void WiFiService::notifyEvent(const WiFiEventData& data) {
    // Call C++ callback
    if (event_callback_) {
        event_callback_(data);
    }
    
    // Send LVGL event
    sendLVGLEvent(data);
}

void WiFiService::sendLVGLEvent(const WiFiEventData& data) {
    if (!ui_event_obj_) return;
    
    // Allocate copy of event data (receiver must delete)
    WiFiEventData* evt_copy = new WiFiEventData(data);
    
    // Send event to LVGL object using lv_obj_send_event
    lv_obj_send_event(ui_event_obj_, LV_EVENT_VALUE_CHANGED, evt_copy);
}

// ============================================================================
// Scan Operations
// ============================================================================

bool WiFiService::startScan() {
    if (!initialized_ || scanning_) return false;
    
    Command cmd;
    cmd.type = CommandType::SCAN;
    
    if (xQueueSend(cmd_queue_, &cmd, 0) != pdTRUE) {
        return false;
    }
    
    scanning_ = true;
    
    WiFiEventData evt;
    evt.event = WiFiEvent::SCAN_STARTED;
    notifyEvent(evt);
    
    return true;
}

bool WiFiService::isScanning() const {
    return scanning_;
}

void WiFiService::doScan() {
    ESP_LOGI(TAG, "Starting WiFi scan...");
    
    // Disconnect if connected to allow full scan (soft disconnect)
    bool was_connected = (WiFi.status() == WL_CONNECTED);
    if (was_connected) {
        WiFi.disconnect(false);  // Don't turn off WiFi
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    
    // Start scan
    int n = WiFi.scanNetworks();
    
    if (n < 0) {
        ESP_LOGE(TAG, "Scan failed: %d", n);
        scanning_ = false;
        
        WiFiEventData evt;
        evt.event = WiFiEvent::SCAN_FAILED;
        evt.reason = n;
        notifyEvent(evt);
        return;
    }
    
    ESP_LOGI(TAG, "Scan found %d networks", n);
    
    // Process results
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        
        scan_count_ = 0;
        for (int i = 0; i < n && scan_count_ < WIFI_SCAN_MAX_AP; i++) {
            WiFiAPInfo& ap = scan_results_[scan_count_];
            ap.clear();
            
            String ssid = WiFi.SSID(i);
            strncpy(ap.ssid, ssid.c_str(), WIFI_SSID_MAX_LEN);
            ap.ssid[WIFI_SSID_MAX_LEN] = '\0';
            
            ap.rssi = WiFi.RSSI(i);
            ap.channel = WiFi.channel(i);
            ap.secured = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            
            // Check if saved
            SettingsStore& store = SettingsStore::instance();
            ap.saved = (store.findSavedNetworkBySsid(ap.ssid) >= 0);
            
            scan_count_++;
        }
        
        xSemaphoreGive(mutex_);
    }
    
    WiFi.scanDelete();
    
    // Reconnect if we were connected
    if (was_connected) {
        doAutoConnect();
    }
    
    scanning_ = false;
    
    WiFiEventData evt;
    evt.event = WiFiEvent::SCAN_COMPLETED;
    notifyEvent(evt);
}

uint8_t WiFiService::getScanResults(std::array<WiFiAPInfo, WIFI_SCAN_MAX_AP>& results) const {
    if (!mutex_) return 0;
    
    xSemaphoreTake(mutex_, portMAX_DELAY);
    uint8_t count = scan_count_;
    results = scan_results_;
    xSemaphoreGive(mutex_);
    
    return count;
}

// ============================================================================
// Connection Operations
// ============================================================================

bool WiFiService::connect(const char* ssid, const char* password, bool save) {
    if (!initialized_ || !ssid) return false;
    
    Command cmd;
    cmd.type = CommandType::CONNECT;
    strncpy(cmd.ssid, ssid, WIFI_SSID_MAX_LEN);
    cmd.ssid[WIFI_SSID_MAX_LEN] = '\0';
    strncpy(cmd.password, password ? password : "", sizeof(cmd.password) - 1);
    cmd.password[sizeof(cmd.password) - 1] = '\0';
    cmd.save = save;
    
    return xQueueSend(cmd_queue_, &cmd, 0) == pdTRUE;
}

bool WiFiService::connectToSaved(uint8_t index) {
    SettingsStore& store = SettingsStore::instance();
    SavedNetwork net = store.getSavedNetwork(index);
    
    if (net.isEmpty()) return false;
    
    return connect(net.ssid, net.password, false);
}

void WiFiService::disconnect() {
    if (!initialized_) return;
    
    Command cmd;
    cmd.type = CommandType::DISCONNECT;
    xQueueSend(cmd_queue_, &cmd, portMAX_DELAY);
}

void WiFiService::doConnect(const char* ssid, const char* password, bool save) {
    ESP_LOGI(TAG, "Connecting to %s...", ssid);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        status_ = WiFiStatus::CONNECTING;
        strncpy(connected_ssid_, ssid, WIFI_SSID_MAX_LEN);
        connected_ssid_[WIFI_SSID_MAX_LEN] = '\0';
        xSemaphoreGive(mutex_);
    }
    
    WiFiEventData evt;
    evt.event = WiFiEvent::CONNECTING;
    strncpy(evt.ssid, ssid, sizeof(evt.ssid));
    notifyEvent(evt);
    
    // Disconnect first (soft disconnect)
    WiFi.disconnect(false);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Start connection
    WiFi.begin(ssid, password);
    
    // Wait for connection with timeout
    uint32_t start = millis();
    wl_status_t status;
    
    while ((status = WiFi.status()) != WL_CONNECTED) {
        if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
            ESP_LOGE(TAG, "Connection timeout");
            WiFi.disconnect();
            
            if (mutex_) {
                xSemaphoreTake(mutex_, portMAX_DELAY);
                status_ = WiFiStatus::CONNECTION_FAILED;
                xSemaphoreGive(mutex_);
            }
            
            WiFiEventData fail_evt;
            fail_evt.event = WiFiEvent::CONNECTION_FAILED;
            strncpy(fail_evt.ssid, ssid, sizeof(fail_evt.ssid));
            fail_evt.reason = -1;  // Timeout
            notifyEvent(fail_evt);
            return;
        }
        
        if (status == WL_CONNECT_FAILED) {
            ESP_LOGE(TAG, "Connection failed");
            
            if (mutex_) {
                xSemaphoreTake(mutex_, portMAX_DELAY);
                status_ = WiFiStatus::CONNECTION_FAILED;
                xSemaphoreGive(mutex_);
            }
            
            WiFiEventData fail_evt;
            fail_evt.event = WiFiEvent::CONNECTION_FAILED;
            strncpy(fail_evt.ssid, ssid, sizeof(fail_evt.ssid));
            fail_evt.reason = -2;  // Auth failed
            notifyEvent(fail_evt);
            return;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Connected!
    ESP_LOGI(TAG, "Connected to %s", ssid);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        status_ = WiFiStatus::CONNECTED;
        rssi_ = WiFi.RSSI();
        strncpy(ip_address_, WiFi.localIP().toString().c_str(), sizeof(ip_address_) - 1);
        ip_address_[sizeof(ip_address_) - 1] = '\0';
        xSemaphoreGive(mutex_);
    }
    
    // Save if requested
    if (save) {
        SavedNetwork net;
        strncpy(net.ssid, ssid, sizeof(net.ssid) - 1);
        strncpy(net.password, password, sizeof(net.password) - 1);
        net.auto_connect = true;
        net.rssi = WiFi.RSSI();
        
        SettingsStore::instance().addSavedNetwork(net);
        SettingsStore::instance().saveNow();  // Immediate save for WiFi
    }
    
    WiFiEventData connected_evt;
    connected_evt.event = WiFiEvent::CONNECTED;
    strncpy(connected_evt.ssid, ssid, sizeof(connected_evt.ssid));
    notifyEvent(connected_evt);
    
    // Also send IP assigned event
    vTaskDelay(pdMS_TO_TICKS(100));  // Wait for IP
    
    if (WiFi.status() == WL_CONNECTED) {
        WiFiEventData ip_evt;
        ip_evt.event = WiFiEvent::IP_ASSIGNED;
        strncpy(ip_evt.ssid, ssid, sizeof(ip_evt.ssid));
        notifyEvent(ip_evt);
    }
}

void WiFiService::doDisconnect() {
    ESP_LOGI(TAG, "Disconnecting...");
    
    WiFi.disconnect(false);  // Soft disconnect
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        status_ = WiFiStatus::DISCONNECTED;
        connected_ssid_[0] = '\0';
        ip_address_[0] = '\0';
        rssi_ = 0;
        xSemaphoreGive(mutex_);
    }
    
    WiFiEventData evt;
    evt.event = WiFiEvent::DISCONNECTED;
    notifyEvent(evt);
}

WiFiStatus WiFiService::getStatus() const {
    if (!mutex_) return WiFiStatus::DISCONNECTED;
    
    xSemaphoreTake(mutex_, portMAX_DELAY);
    WiFiStatus s = status_;
    xSemaphoreGive(mutex_);
    return s;
}

bool WiFiService::isConnected() const {
    return getStatus() == WiFiStatus::CONNECTED;
}

const char* WiFiService::getConnectedSSID() const {
    if (!mutex_) return "";
    
    static char ssid[WIFI_SSID_MAX_LEN + 1];
    xSemaphoreTake(mutex_, portMAX_DELAY);
    strncpy(ssid, connected_ssid_, sizeof(ssid));
    xSemaphoreGive(mutex_);
    return ssid;
}

const char* WiFiService::getIPAddress() const {
    if (!mutex_) return "0.0.0.0";
    
    static char ip[16];
    xSemaphoreTake(mutex_, portMAX_DELAY);
    strncpy(ip, ip_address_, sizeof(ip));
    xSemaphoreGive(mutex_);
    return ip;
}

int8_t WiFiService::getRSSI() const {
    if (!mutex_) return 0;
    
    xSemaphoreTake(mutex_, portMAX_DELAY);
    int8_t r = rssi_;
    xSemaphoreGive(mutex_);
    return r;
}

// ============================================================================
// Saved Networks
// ============================================================================

bool WiFiService::forgetNetwork(const char* ssid) {
    if (!ssid) return false;
    
    // Disconnect if currently connected to this network
    if (strcmp(getConnectedSSID(), ssid) == 0) {
        disconnect();
    }
    
    return SettingsStore::instance().removeSavedNetworkBySsid(ssid);
}

void WiFiService::setAutoConnect(bool enabled) {
    SettingsStore::instance().setWiFiEnabled(enabled);
}

bool WiFiService::autoConnect() {
    if (!initialized_) return false;
    
    Command cmd;
    cmd.type = CommandType::AUTO_CONNECT;
    return xQueueSend(cmd_queue_, &cmd, 0) == pdTRUE;
}

void WiFiService::doAutoConnect() {
    SettingsStore& store = SettingsStore::instance();
    
    uint8_t count = store.getSavedNetworkCount();
    if (count == 0) {
        ESP_LOGI(TAG, "No saved networks to auto-connect");
        return;
    }
    
    ESP_LOGI(TAG, "Auto-connecting to saved networks...");
    
    // Notify connecting state
    WiFiEventData connecting_evt;
    connecting_evt.event = WiFiEvent::CONNECTING;
    notifyEvent(connecting_evt);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        status_ = WiFiStatus::CONNECTING;
        xSemaphoreGive(mutex_);
    }
    
    // Try multiple rounds with increasing delays
    static constexpr uint8_t MAX_RETRY_ROUNDS = 3;
    
    for (uint8_t round = 0; round < MAX_RETRY_ROUNDS; round++) {
        if (round > 0) {
            ESP_LOGI(TAG, "Auto-connect retry round %d/%d", round + 1, MAX_RETRY_ROUNDS);
            vTaskDelay(pdMS_TO_TICKS(2000));  // Wait 2s between rounds
        }
        
        // Try each saved network
        for (uint8_t i = 0; i < count; i++) {
            SavedNetwork net = store.getSavedNetwork(i);
            if (!net.auto_connect || net.isEmpty()) continue;
            
            ESP_LOGI(TAG, "Trying %s...", net.ssid);
            
            // Soft disconnect before connecting (don't reset WiFi hardware)
            WiFi.disconnect(false);
            vTaskDelay(pdMS_TO_TICKS(200));
            
            WiFi.begin(net.ssid, net.password);
            
            // Wait for connection with timeout
            uint32_t start = millis();
            wl_status_t status;
            while ((status = WiFi.status()) != WL_CONNECTED) {
                // Check for timeout
                if (millis() - start > 10000) {  // 10s per network
                    ESP_LOGW(TAG, "Connection to %s timed out", net.ssid);
                    break;
                }
                
                // Check for auth failure
                if (status == WL_CONNECT_FAILED) {
                    ESP_LOGW(TAG, "Connection to %s failed (auth error)", net.ssid);
                    break;
                }
                
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                ESP_LOGI(TAG, "Auto-connected to %s", net.ssid);
                
                if (mutex_) {
                    xSemaphoreTake(mutex_, portMAX_DELAY);
                    status_ = WiFiStatus::CONNECTED;
                    strncpy(connected_ssid_, net.ssid, WIFI_SSID_MAX_LEN);
                    rssi_ = WiFi.RSSI();
                    strncpy(ip_address_, WiFi.localIP().toString().c_str(), sizeof(ip_address_) - 1);
                    xSemaphoreGive(mutex_);
                }
                
                WiFiEventData evt;
                evt.event = WiFiEvent::CONNECTED;
                strncpy(evt.ssid, net.ssid, sizeof(evt.ssid));
                notifyEvent(evt);
                
                // Wait a bit for IP assignment
                vTaskDelay(pdMS_TO_TICKS(500));
                
                if (WiFi.status() == WL_CONNECTED) {
                    WiFiEventData ip_evt;
                    ip_evt.event = WiFiEvent::IP_ASSIGNED;
                    strncpy(ip_evt.ssid, net.ssid, sizeof(ip_evt.ssid));
                    notifyEvent(ip_evt);
                }
                
                return;
            }
            
            // Clean disconnect before trying next network (soft disconnect)
            WiFi.disconnect(false);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
    
    // All retries exhausted
    ESP_LOGW(TAG, "Auto-connect failed after %d rounds", MAX_RETRY_ROUNDS);
    
    if (mutex_) {
        xSemaphoreTake(mutex_, portMAX_DELAY);
        status_ = WiFiStatus::CONNECTION_FAILED;
        xSemaphoreGive(mutex_);
    }
    
    WiFiEventData fail_evt;
    fail_evt.event = WiFiEvent::CONNECTION_FAILED;
    fail_evt.reason = -1;  // General failure
    notifyEvent(fail_evt);
}

} // namespace settings
