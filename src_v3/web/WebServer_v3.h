// =============================================================================
// WebServer_v3.h - Erweiterter WebServer für v3 Protokoll
// =============================================================================
// v3 Web Server für ESP8266 Spider Controller
// Erweitert um:
//   - setWalkParams Command
//   - Servo-Kalibrierungs-Commands (Limits, Center)
//   - GaitRuntime-Integration
// =============================================================================
#ifndef WEB_SERVER_V3_H
#define WEB_SERVER_V3_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncTCP.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// WebSocket Handler
void handleWebSocketV3(AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len);

// Broadcast-Funktionen
void broadcastTerrainStatus();
void broadcastCalibState();
void broadcastWalkParams();
void sendCalibState(AsyncWebSocketClient *client);
void sendWalkParams(AsyncWebSocketClient *client);
void sendServoLimits(AsyncWebSocketClient *client);
void sendAllServoCalib(AsyncWebSocketClient *client);

// Setup
void setupApiRoutes();
void setupStaticFileServing();
void setupWebServer();

// Shutdown
void performShutdown();

// Externe Server-Objekte
extern AsyncWebServer webServer;
extern AsyncWebSocket ws;

#endif // WEB_SERVER_V3_H
