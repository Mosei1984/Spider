#ifndef SPIDER_WEBSERVER_H
#define SPIDER_WEBSERVER_H

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../motion/MotionData.h"

extern AsyncWebServer webServer;
extern AsyncWebSocket ws;

void setupWebServer();
void handleWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                     AwsEventType type, void *arg, uint8_t *data, size_t len);
void broadcastTerrainStatus();
void broadcastCalibState();
void sendCalibState(AsyncWebSocketClient *client);

void setupApiRoutes();
void setupStaticFileServing();

#endif
