#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <functional>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "lvgl.h"

namespace settings {

// ============================================================================
// Constants
// ============================================================================

static constexpr size_t WIFI_SCAN_MAX_AP = 20;
static constexpr size_t WIFI_SSID_MAX_LEN = 32;
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;
static constexpr uint32_t WIFI_SCAN_TIMEOUT_MS = 10000;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief WiFi Access Point info
 */
struct WiFiAPInfo {
    char ssid[WIFI_SSID_MAX_LEN + 1] = {};
    int8_t rssi = 0;
    uint8_t channel = 0;
    bool secured = false;
    bool saved = false;  // In our saved networks list
    
    void clear() { memset(this, 0, sizeof(*this)); }
};

/**
 * @brief WiFi connection status
 */
enum class WiFiStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED,
    SCANNING
};

/**
 * @brief WiFi events for UI notification
 */
enum class WiFiEvent {
    SCAN_STARTED,
    SCAN_COMPLETED,
    SCAN_FAILED,
    CONNECTING,
    CONNECTED,
    DISCONNECTED,
    CONNECTION_FAILED,
    IP_ASSIGNED,
    RSSI_UPDATED
};

/**
 * @brief WiFi event data sent to UI
 */
struct WiFiEventData {
    WiFiEvent event;
    char ssid[WIFI_SSID_MAX_LEN + 1];
    int32_t reason;  // Error code for failures
    
    WiFiEventData() : event(WiFiEvent::DISCONNECTED), reason(0) {
        ssid[0] = '\0';
    }
};

// ============================================================================
// WiFi Service
// ============================================================================

/**
 * @brief Async WiFi service with event-driven UI updates
 * 
 * All operations are non-blocking. Results are delivered via:
 * 1. Callbacks registered with setEventCallback()
 * 2. LVGL events sent to a registered UI object
 * 
 * The service runs its own task for blocking operations (scan, connect).
 */
class WiFiService {
public:
    static WiFiService& instance();
    
    // Disable copy/move
    WiFiService(const WiFiService&) = delete;
    WiFiService& operator=(const WiFiService&) = delete;
    
    /**
     * @brief Initialize WiFi service
     * @param ui_event_obj LVGL object to receive events (can be nullptr)
     * @return true if successful
     */
    bool init(lv_obj_t* ui_event_obj = nullptr);
    
    /**
     * @brief Deinitialize WiFi
     */
    void deinit();
    
    /**
     * @brief Check if initialized
     */
    bool isInitialized() const { return initialized_; }
    
    // ============================================================================
    // Event Handling
    // ============================================================================
    
    using EventCallback = std::function<void(const WiFiEventData&)>;
    
    /**
     * @brief Set callback for WiFi events
     * Called from WiFi task context - keep it short!
     */
    void setEventCallback(EventCallback callback);
    
    /**
     * @brief Register LVGL object to receive LV_EVENT_USER_DATA events
     * Event user_data will be pointer to WiFiEventData (copy before use)
     */
    void setUIEventObject(lv_obj_t* obj);
    
    // ============================================================================
    // Scan Operations
    // ============================================================================
    
    /**
     * @brief Start async WiFi scan
     * @return true if scan started
     */
    bool startScan();
    
    /**
     * @brief Check if scan is in progress
         */
    bool isScanning() const;
    
    /**
     * @brief Get scan results
     * @return Number of APs found
     */
    uint8_t getScanResults(std::array<WiFiAPInfo, WIFI_SCAN_MAX_AP>& results) const;
    
    /**
     * @brief Get cached scan result count
     */
    uint8_t getCachedScanCount() const { return scan_count_; }
    
    // ============================================================================
    // Connection Operations
    // ============================================================================
    
    /**
     * @brief Connect to WiFi network
     * @param ssid Network SSID
     * @param password Network password (empty for open networks)
     * @param save Save to persistent storage if successful
     * @return true if connection process started
     */
    bool connect(const char* ssid, const char* password, bool save = true);
    
    /**
     * @brief Connect to saved network by index
     */
    bool connectToSaved(uint8_t index);
    
    /**
     * @brief Disconnect from current network
     */
    void disconnect();
    
    /**
     * @brief Get current connection status
     */
    WiFiStatus getStatus() const;
    
    /**
     * @brief Check if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Get current SSID
     * @return SSID or empty string if not connected
     */
    const char* getConnectedSSID() const;
    
    /**
     * @brief Get current IP address
     * @return IP as string or "0.0.0.0"
     */
    const char* getIPAddress() const;
    
    /**
     * @brief Get current RSSI
     * @return RSSI in dBm or 0 if not connected
     */
    int8_t getRSSI() const;
    
    // ============================================================================
    // Saved Networks
    // ============================================================================
    
    /**
     * @brief Forget a saved network
     */
    bool forgetNetwork(const char* ssid);
    
    /**
     * @brief Enable/disable auto-connect on boot
     */
    void setAutoConnect(bool enabled);
    
    /**
     * @brief Try to connect to any saved network
     */
    bool autoConnect();

private:
    WiFiService() = default;
    ~WiFiService() = default;
    
    // Task
    static void taskEntry(void* param);
    void runTask();
    
    // Command types for task queue
    enum class CommandType {
        SCAN,
        CONNECT,
        DISCONNECT,
        FORGET,
        AUTO_CONNECT,
        STOP
    };
    
    struct Command {
        CommandType type;
        char ssid[WIFI_SSID_MAX_LEN + 1];
        char password[64];
        bool save;
        uint8_t saved_index;
    };
    
    // Internal methods
    void processCommand(const Command& cmd);
    void doScan();
    void doConnect(const char* ssid, const char* password, bool save);
    void doDisconnect();
    void doAutoConnect();
    void updateScanResults();
    void notifyEvent(const WiFiEventData& data);
    void sendLVGLEvent(const WiFiEventData& data);
    
    // State
    bool initialized_ = false;
    TaskHandle_t task_handle_ = nullptr;
    QueueHandle_t cmd_queue_ = nullptr;
    
    mutable SemaphoreHandle_t mutex_ = nullptr;
    
    // Scan results
    std::array<WiFiAPInfo, WIFI_SCAN_MAX_AP> scan_results_;
    uint8_t scan_count_ = 0;
    volatile bool scanning_ = false;
    
    // Connection state
    WiFiStatus status_ = WiFiStatus::DISCONNECTED;
    char connected_ssid_[WIFI_SSID_MAX_LEN + 1] = {};
    char ip_address_[16] = "0.0.0.0";
    int8_t rssi_ = 0;
    
    // Callbacks
    EventCallback event_callback_;
    lv_obj_t* ui_event_obj_ = nullptr;
};

} // namespace settings
