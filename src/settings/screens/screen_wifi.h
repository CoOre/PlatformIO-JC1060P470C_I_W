#pragma once

#include "../navigation/ui_router.h"
#include "../services/wifi_service.h"
#include <array>

namespace settings {

/**
 * @brief WiFi settings screen
 * 
 * Features:
 * - Current connection status (SSID, IP, RSSI)
 * - Scan for networks
 * - Connect/disconnect
 * - Saved networks list
 * - Password entry with on-screen keyboard
 */
class WiFiScreen : public Screen {
public:
    WiFiScreen() = default;
    ~WiFiScreen() override = default;
    
    bool create(lv_obj_t* parent) override;
    void destroy() override;
    void update(uint32_t now_ms) override;
    bool onBack() override;
    ScreenType getType() const override { return ScreenType::WIFI; }

private:
    void createHeader();
    void createStatusSection();
    void createNetworksList();
    void createPasswordDialog();
    void createSavedNetworksSection();
    
    void refreshStatus();
    void refreshNetworksList();
    void refreshSavedNetworks();
    
    void showPasswordDialog(const char* ssid);
    void hidePasswordDialog();
    
    void onConnectClicked(const char* ssid);
    void onDisconnectClicked();
    void onForgetClicked(const char* ssid);
    void onPasswordEntered();
    
    static void onScanClicked(lv_event_t* e);
    static void onNetworkClicked(lv_event_t* e);
    static void onSavedNetworkClicked(lv_event_t* e);
    static void onPasswordConfirmClicked(lv_event_t* e);
    static void onPasswordCancelClicked(lv_event_t* e);
    static void onEventReceived(lv_event_t* e);
    
    void handleWiFiEvent(const WiFiEventData& evt);
    
    // UI Elements
    lv_obj_t* header_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* ssid_label_ = nullptr;
    lv_obj_t* ip_label_ = nullptr;
    lv_obj_t* rssi_label_ = nullptr;
    lv_obj_t* scan_btn_ = nullptr;
    lv_obj_t* networks_list_ = nullptr;
    lv_obj_t* saved_list_ = nullptr;
    
    // Password dialog
    lv_obj_t* password_dialog_ = nullptr;
    lv_obj_t* password_ta_ = nullptr;
    lv_obj_t* password_kb_ = nullptr;
    char pending_ssid_[WIFI_SSID_MAX_LEN + 1] = {};
    
    // State
    std::array<WiFiAPInfo, WIFI_SCAN_MAX_AP> scan_results_;
    uint8_t scan_count_ = 0;
    bool showing_password_dialog_ = false;
};

} // namespace settings
