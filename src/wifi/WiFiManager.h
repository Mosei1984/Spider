#ifndef SPIDER_WIFIMANAGER_H
#define SPIDER_WIFIMANAGER_H

#include <ESP8266WiFi.h>

// WiFi-Konfiguration
struct WiFiConfig {
    const char* staSsid;      // Heim-WLAN SSID
    const char* staPassword;  // Heim-WLAN Passwort
    const char* apSsid;       // AP-Mode SSID (Fallback)
    const char* apPassword;   // AP-Mode Passwort
    uint8_t apChannel;        // AP Kanal
    uint32_t staTimeout;      // Verbindungs-Timeout in ms
};

class WiFiManager {
public:
    WiFiManager();
    
    // Konfiguration setzen
    void setConfig(const WiFiConfig& config);
    
    // WiFi starten (versucht STA, Fallback zu AP)
    bool begin();
    
    // Aktueller Modus
    bool isStationMode() const { return _isStation; }
    
    // IP-Adresse
    IPAddress getIP() const;

private:
    bool connectStation();
    void startAccessPoint();
    
    WiFiConfig _config;
    bool _isStation;
};

extern WiFiManager wifiManager;

#endif
