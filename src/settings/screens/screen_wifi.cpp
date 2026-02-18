#include "screen_wifi.h"
#include "../i18n/i18n.h"
#include "../core/settings_store.h"
#include "esp_log.h"

namespace settings {

static const char* TAG = "WiFiScreen";

// ============================================================================
// Lifecycle
// ============================================================================

bool WiFiScreen::create(lv_obj_t* parent) {
    if (!parent) return false;
    
    setupContainer(parent);
    
    createHeader();
    createStatusSection();
    createNetworksList();
    createSavedNetworksSection();
    createPasswordDialog();
    
    // Register for WiFi events
    WiFiService::instance().setUIEventObject(container_);
    lv_obj_add_event_cb(container_, onEventReceived, LV_EVENT_VALUE_CHANGED, this);
    
    // Initial refresh
    refreshStatus();
    refreshSavedNetworks();
    
    ESP_LOGI(TAG, "WiFi screen created");
    return true;
}

void WiFiScreen::destroy() {
    // Unregister from WiFi events
    WiFiService::instance().setUIEventObject(nullptr);
    
    if (container_) {
        lv_obj_del(container_);
        container_ = nullptr;
    }
    
    ESP_LOGI(TAG, "WiFi screen destroyed");
}

void WiFiScreen::update(uint32_t now_ms) {
    // Periodic updates if needed
}

bool WiFiScreen::onBack() {
    if (showing_password_dialog_) {
        hidePasswordDialog();
        return true;
    }
    return false;
}

// ============================================================================
// UI Creation
// ============================================================================

void WiFiScreen::createHeader() {
    header_ = lv_obj_create(container_);
    lv_obj_set_size(header_, LV_PCT(100), 50);
    lv_obj_set_style_pad_all(header_, 10, LV_PART_MAIN);
    lv_obj_set_style_border_width(header_, 0, LV_PART_MAIN);
    
    lv_obj_t* title = lv_label_create(header_);
    lv_label_set_text(title, S(WIFI_TITLE));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_center(title);
}

void WiFiScreen::createStatusSection() {
    lv_obj_t* section = lv_obj_create(container_);
    lv_obj_set_size(section, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(section, 10, LV_PART_MAIN);
    
    // Status
    status_label_ = lv_label_create(section);
    lv_label_set_text(status_label_, S(WIFI_DISCONNECTED));
    lv_obj_align(status_label_, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // SSID
    ssid_label_ = lv_label_create(section);
    lv_label_set_text(ssid_label_, "");
    lv_obj_align(ssid_label_, LV_ALIGN_TOP_LEFT, 0, 25);
    
    // IP
    ip_label_ = lv_label_create(section);
    lv_label_set_text(ip_label_, "");
    lv_obj_align(ip_label_, LV_ALIGN_TOP_LEFT, 0, 50);
    
    // RSSI
    rssi_label_ = lv_label_create(section);
    lv_label_set_text(rssi_label_, "");
    lv_obj_align(rssi_label_, LV_ALIGN_TOP_LEFT, 0, 75);
    
    // Disconnect button (hidden by default)
    lv_obj_t* disconnect_btn = lv_btn_create(section);
    lv_obj_set_size(disconnect_btn, 120, 40);
    lv_obj_align(disconnect_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_flag(disconnect_btn, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_t* btn_label = lv_label_create(disconnect_btn);
    lv_label_set_text(btn_label, S(DISCONNECT));
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(disconnect_btn, [](lv_event_t* e) {
        WiFiService::instance().disconnect();
    }, LV_EVENT_CLICKED, nullptr);
}

void WiFiScreen::createNetworksList() {
    // Scan button
    scan_btn_ = lv_btn_create(container_);
    lv_obj_set_size(scan_btn_, LV_PCT(100), 50);
    lv_obj_set_style_bg_color(scan_btn_, lv_color_hex(0x2196F3), LV_PART_MAIN);
    
    lv_obj_t* btn_label = lv_label_create(scan_btn_);
    lv_label_set_text(btn_label, S(WIFI_SCAN));
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(scan_btn_, onScanClicked, LV_EVENT_CLICKED, this);
    
    // Networks list
    networks_list_ = lv_list_create(container_);
    lv_obj_set_size(networks_list_, LV_PCT(100), 200);
    lv_obj_set_style_border_width(networks_list_, 1, LV_PART_MAIN);
    
    // Placeholder label
    lv_obj_t* placeholder = lv_label_create(networks_list_);
    lv_label_set_text(placeholder, S(WIFI_NO_NETWORKS));
}

void WiFiScreen::createSavedNetworksSection() {
    lv_obj_t* label = lv_label_create(container_);
    lv_label_set_text(label, S(WIFI_SAVED_NETWORKS));
    lv_obj_set_style_text_font(label, &lv_font_roboto_14, LV_PART_MAIN);
    
    saved_list_ = lv_list_create(container_);
    lv_obj_set_size(saved_list_, LV_PCT(100), 150);
    lv_obj_set_style_border_width(saved_list_, 1, LV_PART_MAIN);
}

void WiFiScreen::createPasswordDialog() {
    password_dialog_ = lv_obj_create(container_);
    lv_obj_set_size(password_dialog_, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(password_dialog_, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_flag(password_dialog_, LV_OBJ_FLAG_HIDDEN);
    
    // Title
    lv_obj_t* title = lv_label_create(password_dialog_);
    lv_label_set_text(title, S(WIFI_ENTER_PASSWORD));
    lv_obj_set_style_text_font(title, &lv_font_roboto_18, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    // SSID label
    lv_obj_t* ssid_lbl = lv_label_create(password_dialog_);
    lv_label_set_text(ssid_lbl, "");
    lv_obj_set_user_data(ssid_lbl, this);
    lv_obj_align(ssid_lbl, LV_ALIGN_TOP_MID, 0, 50);
    
    // Password textarea
    password_ta_ = lv_textarea_create(password_dialog_);
    lv_obj_set_size(password_ta_, 300, 50);
    lv_obj_align(password_ta_, LV_ALIGN_TOP_MID, 0, 80);
    lv_textarea_set_password_mode(password_ta_, true);
    lv_textarea_set_placeholder_text(password_ta_, S(WIFI_PASSWORD));
    
    // Show password checkbox
    lv_obj_t* show_cb = lv_checkbox_create(password_dialog_);
    lv_checkbox_set_text(show_cb, S(WIFI_SHOW_PASSWORD));
    lv_obj_align(show_cb, LV_ALIGN_TOP_MID, 0, 140);
    
    lv_obj_add_event_cb(show_cb, [](lv_event_t* e) {
        lv_obj_t* cb = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
        WiFiScreen* screen = static_cast<WiFiScreen*>(lv_obj_get_user_data(cb));
        lv_state_t state = lv_obj_get_state(cb);
        bool checked = (state & LV_STATE_CHECKED);
        lv_textarea_set_password_mode(screen->password_ta_, !checked);
    }, LV_EVENT_VALUE_CHANGED, this);
    
    // Buttons
    lv_obj_t* btn_container = lv_obj_create(password_dialog_);
    lv_obj_set_size(btn_container, 300, 50);
    lv_obj_align(btn_container, LV_ALIGN_TOP_MID, 0, 180);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(btn_container, 20, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn_container, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, LV_PART_MAIN);
    
    lv_obj_t* cancel_btn = lv_btn_create(btn_container);
    lv_obj_set_size(cancel_btn, 130, 40);
    lv_obj_t* cancel_lbl = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_lbl, S(CANCEL));
    lv_obj_center(cancel_lbl);
    lv_obj_add_event_cb(cancel_btn, onPasswordCancelClicked, LV_EVENT_CLICKED, this);
    
    lv_obj_t* connect_btn = lv_btn_create(btn_container);
    lv_obj_set_size(connect_btn, 130, 40);
    lv_obj_set_style_bg_color(connect_btn, lv_color_hex(0x4CAF50), LV_PART_MAIN);
    lv_obj_t* connect_lbl = lv_label_create(connect_btn);
    lv_label_set_text(connect_lbl, S(CONNECT));
    lv_obj_center(connect_lbl);
    lv_obj_add_event_cb(connect_btn, onPasswordConfirmClicked, LV_EVENT_CLICKED, this);
    
    // Keyboard
    password_kb_ = lv_keyboard_create(password_dialog_);
    lv_obj_set_size(password_kb_, LV_PCT(100), 200);
    lv_obj_align(password_kb_, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(password_kb_, password_ta_);
}

// ============================================================================
// UI Updates
// ============================================================================

void WiFiScreen::refreshStatus() {
    WiFiService& wifi = WiFiService::instance();
    WiFiStatus status = wifi.getStatus();
    
    switch (status) {
        case WiFiStatus::CONNECTED:
            lv_label_set_text(status_label_, S(WIFI_CONNECTED));
            lv_label_set_text_fmt(ssid_label_, "%s: %s", S(WIFI_SSID), wifi.getConnectedSSID());
            lv_label_set_text_fmt(ip_label_, "%s: %s", S(WIFI_IP_ADDRESS), wifi.getIPAddress());
            lv_label_set_text_fmt(rssi_label_, "%s: %d dBm", S(WIFI_SIGNAL), wifi.getRSSI());
            break;
            
        case WiFiStatus::CONNECTING:
            lv_label_set_text(status_label_, S(WIFI_CONNECTING));
            lv_label_set_text(ssid_label_, "");
            lv_label_set_text(ip_label_, "");
            lv_label_set_text(rssi_label_, "");
            break;
            
        case WiFiStatus::CONNECTION_FAILED:
            lv_label_set_text(status_label_, S(WIFI_FAILED));
            break;
            
        default:
            lv_label_set_text(status_label_, S(WIFI_DISCONNECTED));
            lv_label_set_text(ssid_label_, "");
            lv_label_set_text(ip_label_, "");
            lv_label_set_text(rssi_label_, "");
            break;
    }
}

void WiFiScreen::refreshNetworksList() {
    // Clear list
    lv_obj_clean(networks_list_);
    
    if (scan_count_ == 0) {
        lv_obj_t* placeholder = lv_label_create(networks_list_);
        lv_label_set_text(placeholder, S(WIFI_NO_NETWORKS));
        return;
    }
    
    // Add networks
    for (uint8_t i = 0; i < scan_count_; i++) {
        const WiFiAPInfo& ap = scan_results_[i];
        
        // Format: "SSID [-65 dBm]" or "SSID [SAVED]"
        char buf[64];
        if (ap.saved) {
            snprintf(buf, sizeof(buf), "%s [%s]", ap.ssid, "Saved");
        } else {
            snprintf(buf, sizeof(buf), "%s [%d dBm] %s", 
                     ap.ssid, ap.rssi, ap.secured ? "\xEF\x80\xA3" : "");
        }
        
        lv_obj_t* btn = lv_list_add_btn(networks_list_, nullptr, buf);
        
        // Store SSID in user data
        char* ssid_copy = new char[strlen(ap.ssid) + 1];
        strcpy(ssid_copy, ap.ssid);
        lv_obj_set_user_data(btn, ssid_copy);
        
        lv_obj_add_event_cb(btn, onNetworkClicked, LV_EVENT_CLICKED, this);
    }
}

void WiFiScreen::refreshSavedNetworks() {
    lv_obj_clean(saved_list_);
    
    SettingsStore& store = SettingsStore::instance();
    uint8_t count = store.getSavedNetworkCount();
    
    for (uint8_t i = 0; i < count; i++) {
        SavedNetwork net = store.getSavedNetwork(i);
        if (net.isEmpty()) continue;
        
        lv_obj_t* btn = lv_list_add_btn(saved_list_, nullptr, net.ssid);
        
        char* ssid_copy = new char[strlen(net.ssid) + 1];
        strcpy(ssid_copy, net.ssid);
        lv_obj_set_user_data(btn, ssid_copy);
        
        lv_obj_add_event_cb(btn, onSavedNetworkClicked, LV_EVENT_CLICKED, this);
    }
}

// ============================================================================
// Password Dialog
// ============================================================================

void WiFiScreen::showPasswordDialog(const char* ssid) {
    strncpy(pending_ssid_, ssid, sizeof(pending_ssid_) - 1);
    pending_ssid_[sizeof(pending_ssid_) - 1] = '\0';
    
    // Update SSID label in dialog
    lv_obj_t* ssid_lbl = lv_obj_get_child(password_dialog_, 1);
    lv_label_set_text_fmt(ssid_lbl, "%s: %s", S(WIFI_SSID), ssid);
    
    // Clear password
    lv_textarea_set_text(password_ta_, "");
    
    lv_obj_clear_flag(password_dialog_, LV_OBJ_FLAG_HIDDEN);
    showing_password_dialog_ = true;
}

void WiFiScreen::hidePasswordDialog() {
    lv_obj_add_flag(password_dialog_, LV_OBJ_FLAG_HIDDEN);
    showing_password_dialog_ = false;
    pending_ssid_[0] = '\0';
}

// ============================================================================
// Event Handlers
// ============================================================================

void WiFiScreen::onScanClicked(lv_event_t* e) {
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    
    lv_label_set_text(static_cast<lv_obj_t*>(lv_obj_get_child(screen->scan_btn_, 0)), 
                      S(SCANNING));
    lv_obj_add_state(screen->scan_btn_, LV_STATE_DISABLED);
    
    WiFiService::instance().startScan();
}

void WiFiScreen::onNetworkClicked(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    
    const char* ssid = static_cast<const char*>(lv_obj_get_user_data(btn));
    if (!ssid) return;
    
    // Check if already saved
    SettingsStore& store = SettingsStore::instance();
    int8_t saved_idx = store.findSavedNetworkBySsid(ssid);
    
    if (saved_idx >= 0) {
        // Auto-connect using saved password
        WiFiService::instance().connectToSaved(saved_idx);
    } else {
        // Show password dialog
        screen->showPasswordDialog(ssid);
    }
}

void WiFiScreen::onSavedNetworkClicked(lv_event_t* e) {
    lv_obj_t* btn = static_cast<lv_obj_t*>(lv_event_get_target_obj(e));
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    
    const char* ssid = static_cast<const char*>(lv_obj_get_user_data(btn));
    if (!ssid) return;
    
    // Show menu: Connect / Forget
    // For simplicity, just connect
    SettingsStore& store = SettingsStore::instance();
    int8_t idx = store.findSavedNetworkBySsid(ssid);
    if (idx >= 0) {
        WiFiService::instance().connectToSaved(idx);
    }
}

void WiFiScreen::onPasswordConfirmClicked(lv_event_t* e) {
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    
    const char* password = lv_textarea_get_text(screen->password_ta_);
    WiFiService::instance().connect(screen->pending_ssid_, password, true);
    
    screen->hidePasswordDialog();
}

void WiFiScreen::onPasswordCancelClicked(lv_event_t* e) {
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    screen->hidePasswordDialog();
}

void WiFiScreen::onEventReceived(lv_event_t* e) {
    WiFiScreen* screen = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    WiFiEventData* evt = static_cast<WiFiEventData*>(lv_event_get_param(e));
    
    if (evt) {
        screen->handleWiFiEvent(*evt);
        delete evt;
    }
}

void WiFiScreen::handleWiFiEvent(const WiFiEventData& evt) {
    switch (evt.event) {
        case WiFiEvent::SCAN_COMPLETED:
            scan_count_ = WiFiService::instance().getScanResults(scan_results_);
            refreshNetworksList();
            lv_label_set_text(static_cast<lv_obj_t*>(lv_obj_get_child(scan_btn_, 0)), 
                              S(WIFI_SCAN));
            lv_obj_clear_state(scan_btn_, LV_STATE_DISABLED);
            break;
            
        case WiFiEvent::SCAN_FAILED:
            lv_label_set_text(static_cast<lv_obj_t*>(lv_obj_get_child(scan_btn_, 0)), 
                              S(WIFI_SCAN));
            lv_obj_clear_state(scan_btn_, LV_STATE_DISABLED);
            break;
            
        case WiFiEvent::CONNECTED:
        case WiFiEvent::DISCONNECTED:
        case WiFiEvent::CONNECTION_FAILED:
        case WiFiEvent::IP_ASSIGNED:
            refreshStatus();
            break;
            
        default:
            break;
    }
}

} // namespace settings
