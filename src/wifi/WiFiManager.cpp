#include "WiFiManager.h"

WiFiManager wifiManager;

WiFiManager::WiFiManager() : _isStation(false) {
    _config = {
        nullptr,    // staSsid
        nullptr,    // staPassword
        "QuadBot-E", // apSsid
        "12345678",  // apPassword
        5,           // apChannel
        10000        // staTimeout (10s)
    };
}

void WiFiManager::setConfig(const WiFiConfig& config) {
    _config = config;
}

bool WiFiManager::begin() {
    WiFi.persistent(false);
    WiFi.disconnect(true);
    delay(100);
    
    // Versuche Station-Mode wenn Heim-WLAN konfiguriert
    if (_config.staSsid != nullptr && strlen(_config.staSsid) > 0) {
        if (connectStation()) {
            return true;
        }
        Serial.println("[WiFi] STA-Verbindung fehlgeschlagen, starte AP-Mode...");
    }
    
    // Fallback zu Access Point
    startAccessPoint();
    return true;
}

bool WiFiManager::connectStation() {
    Serial.printf("[WiFi] Verbinde mit '%s'...\n", _config.staSsid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(_config.staSsid, _config.staPassword);
    
    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > _config.staTimeout) {
            Serial.println("[WiFi] Timeout!");
            return false;
        }
        delay(500);
        Serial.print(".");
    }
    
    Serial.println();
    Serial.printf("[WiFi] Verbunden! IP: %s\n", WiFi.localIP().toString().c_str());
    _isStation = true;
    return true;
}

void WiFiManager::startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(_config.apSsid, _config.apPassword, _config.apChannel);
    
    Serial.printf("[WiFi] AP gestartet: %s\n", _config.apSsid);
    Serial.printf("[WiFi] AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    _isStation = false;
}

IPAddress WiFiManager::getIP() const {
    return _isStation ? WiFi.localIP() : WiFi.softAPIP();
}
