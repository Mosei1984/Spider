#include "WebServer.h"
#include "../robot/RobotController.h"
#include "../motion/MotionData.h"

extern RobotController robotController;

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

static String getContentType(const String& path) {
    if (path.endsWith(".html")) return "text/html";
    if (path.endsWith(".css")) return "text/css";
    if (path.endsWith(".js")) return "application/javascript";
    if (path.endsWith(".json")) return "application/json";
    if (path.endsWith(".png")) return "image/png";
    if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
    if (path.endsWith(".gif")) return "image/gif";
    if (path.endsWith(".svg")) return "image/svg+xml";
    if (path.endsWith(".ico")) return "image/x-icon";
    if (path.endsWith(".woff")) return "font/woff";
    if (path.endsWith(".woff2")) return "font/woff2";
    return "text/plain";
}

void handleWebSocket(AsyncWebSocket *server, AsyncWebSocketClient *client,
                     AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n", client->id(), 
                          client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA: {
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                data[len] = 0;
                Serial.printf("[WS] Received: %s\n", (char*)data);
                
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, data, len);
                if (!error) {
                    const char* type = doc["type"];
                    if (type) {
                        if (strcmp(type, "cmd") == 0) {
                            const char* name = doc["name"];
                            if (name) {
                                Serial.printf("[WS] Queuing command: %s\n", name);
                                robotController.queueCommand(robotController.parseCommand(name));
                            }
                        } else if (strcmp(type, "moveStart") == 0) {
                            const char* name = doc["name"];
                            if (name) {
                                Serial.printf("[WS] Kontinuierliche Bewegung: %s\n", name);
                                robotController.startContinuous(robotController.parseCommand(name));
                            }
                        } else if (strcmp(type, "moveStop") == 0) {
                            Serial.println("[WS] Bewegung stoppen");
                            robotController.requestStop();
                        } else if (strcmp(type, "stop") == 0) {
                            Serial.println("[WS] Sofortiger Stop");
                            robotController.forceStop();
                        } else if (strcmp(type, "setOffsets") == 0) {
                            JsonArray offsets = doc["offsets"];
                            if (!offsets.isNull()) {
                                int i = 0;
                                for (JsonVariant v : offsets) {
                                    if (i < ALLSERVOS) {
                                        robotController.setServoOffset(i, v.as<int>());
                                    }
                                    i++;
                                }
                                saveOffsetsToFile();
                                Serial.println("[WS] Offsets updated and saved");
                            }
                        } else if (strcmp(type, "setSpeed") == 0) {
                            if (doc["speed"].is<int>()) {
                                int speed = doc["speed"].as<int>();
                                setSpeed(speed);
                                Serial.printf("[WS] Speed set to %d%% (timePercent=%d%%)\n", speed, (110 - speed) / 3);
                            } else {
                                Serial.println("[WS] setSpeed: missing 'speed' field!");
                            }
                        } else if (strcmp(type, "setTerrain") == 0) {
                            const char* mode = doc["mode"];
                            if (mode) {
                                if (strcmp(mode, "uphill") == 0) {
                                    setTerrainMode(TERRAIN_UPHILL);
                                } else if (strcmp(mode, "downhill") == 0) {
                                    setTerrainMode(TERRAIN_DOWNHILL);
                                } else {
                                    setTerrainMode(TERRAIN_NORMAL);
                                }
                                broadcastTerrainStatus();
                            }
                        } else if (strcmp(type, "getOffsets") == 0) {
                            JsonDocument resp;
                            JsonArray arr = resp["offsets"].to<JsonArray>();
                            for (int i = 0; i < ALLSERVOS; i++) {
                                arr.add(robotController.getServoOffset(i));
                            }
                            String out;
                            serializeJson(resp, out);
                            client->text(out);
                            Serial.println("[WS] Offsets sent to client");
                        }
                    }
                }
            }
            break;
        }
        case WS_EVT_PONG:
        case WS_EVT_PING:
        case WS_EVT_ERROR:
            break;
    }
}

void broadcastStatus() {
    JsonDocument doc;
    doc["type"] = "status";
    doc["connected"] = true;
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void broadcastTerrainStatus() {
    JsonDocument doc;
    doc["type"] = "terrain";
    doc["mode"] = getTerrainModeName();
    doc["modeId"] = (int)getTerrainMode();
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void setupApiRoutes() {
    webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["uptime"] = millis();
        doc["clients"] = ws.count();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    webServer.on("/api/cmd", HTTP_POST, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            const char* name = doc["name"];
            if (!name) {
                request->send(400, "application/json", "{\"error\":\"Missing 'name' field\"}");
                return;
            }
            
            Serial.printf("[API] Queuing command: %s\n", name);
            robotController.queueCommand(robotController.parseCommand(name));
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    webServer.on("/api/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        Serial.println("[API] Force stop");
        robotController.forceStop();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    webServer.on("/api/calibration", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["offsets"] = JsonArray();
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    webServer.on("/api/calibration", HTTP_PUT, [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, data, len);
            
            if (error) {
                request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
                return;
            }
            
            JsonArray offsets = doc["offsets"];
            if (offsets.isNull()) {
                request->send(400, "application/json", "{\"error\":\"Missing 'offsets' field\"}");
                return;
            }
            
            Serial.println("[API] Calibration update");
            
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );
}

void setupStaticFileServing() {
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(200, "text/html", "<h1>Spider Bot</h1><p>Upload web files to LittleFS</p>");
        }
    });

    webServer.serveStatic("/", LittleFS, "/").setCacheControl("max-age=86400");

    webServer.onNotFound([](AsyncWebServerRequest *request) {
        String path = request->url();
        
        if (path.startsWith("/api/")) {
            request->send(404, "application/json", "{\"error\":\"Not found\"}");
            return;
        }

        String gzPath = path + ".gz";
        if (LittleFS.exists(gzPath)) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, gzPath, getContentType(path));
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
            return;
        }

        if (LittleFS.exists(path)) {
            request->send(LittleFS, path, getContentType(path));
            return;
        }

        if (LittleFS.exists("/index.html.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(404, "text/plain", "Not found");
        }
    });
}

void setupWebServer() {
    if (!LittleFS.begin()) {
        Serial.println("[FS] LittleFS mount failed!");
    } else {
        Serial.println("[FS] LittleFS mounted");
        loadOffsetsFromFile();
    }

    ws.onEvent(handleWebSocket);
    webServer.addHandler(&ws);

    setupApiRoutes();
    setupStaticFileServing();

    webServer.begin();
    Serial.println("[Web] Server started on port 80");
}
