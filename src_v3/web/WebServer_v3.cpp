// =============================================================================
// WebServer_v3.cpp - Implementierung des erweiterten WebServers
// =============================================================================
#include "WebServer_v3.h"
#include "../robot/RobotController_v3.h"
#include "../motion/MotionData_v3.h"
#include "../gait/GaitRuntime.h"
#include "../calibration/ServoCalibration.h"

extern RobotControllerV3 robotController;

AsyncWebServer webServer(80);
AsyncWebSocket ws("/ws");

// =============================================================================
// Content-Type Helper
// =============================================================================
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
    return "text/plain";
}

// =============================================================================
// WebSocket Handler - Erweitert für v3 Commands
// =============================================================================
void handleWebSocketV3(AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("[WS] Client #%u connected from %s\n", client->id(), 
                          client->remoteIP().toString().c_str());
            sendCalibState(client);
            sendWalkParams(client);
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
                    const char* msgType = doc["type"];
                    if (msgType) {
                        // ========== Original Commands ==========
                        if (strcmp(msgType, "cmd") == 0) {
                            const char* name = doc["name"];
                            if (name) {
                                robotController.queueCommand(robotController.parseCommand(name));
                            }
                        }
                        else if (strcmp(msgType, "moveStart") == 0) {
                            const char* name = doc["name"];
                            if (name) {
                                // Optionale Parameter-Overrides prüfen
                                if (doc.containsKey("stride")) {
                                    robotController.setStrideFactor(doc["stride"].as<float>());
                                }
                                if (doc.containsKey("subSteps")) {
                                    robotController.setSubSteps(doc["subSteps"].as<uint8_t>());
                                }
                                robotController.startContinuous(robotController.parseCommand(name));
                            }
                        }
                        else if (strcmp(msgType, "moveStop") == 0) {
                            robotController.requestStop();
                        }
                        else if (strcmp(msgType, "stop") == 0) {
                            robotController.forceStop();
                        }
                        else if (strcmp(msgType, "setSpeed") == 0) {
                            if (doc["speed"].is<int>()) {
                                setSpeed(doc["speed"].as<int>());
                            }
                        }
                        else if (strcmp(msgType, "setTerrain") == 0) {
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
                        }
                        
                        // ========== v3 Walk-Parameter Commands ==========
                        else if (strcmp(msgType, "setWalkParams") == 0) {
                            WalkParams params;
                            if (doc.containsKey("stride")) params.stride = doc["stride"].as<float>();
                            if (doc.containsKey("subSteps")) params.subSteps = doc["subSteps"].as<uint8_t>();
                            if (doc.containsKey("profile")) params.profile = (TimingProfile)doc["profile"].as<int>();
                            if (doc.containsKey("swingMul")) params.swingMul = doc["swingMul"].as<float>();
                            if (doc.containsKey("stanceMul")) params.stanceMul = doc["stanceMul"].as<float>();
                            if (doc.containsKey("rampEnabled")) params.rampEnabled = doc["rampEnabled"].as<bool>();
                            if (doc.containsKey("rampCycles")) params.rampCycles = doc["rampCycles"].as<uint8_t>();
                            
                            robotController.setWalkParams(params);
                            broadcastWalkParams();
                            Serial.println(F("[WS] WalkParams updated"));
                        }
                        else if (strcmp(msgType, "setStride") == 0) {
                            if (doc.containsKey("value")) {
                                robotController.setStrideFactor(doc["value"].as<float>());
                            }
                        }
                        else if (strcmp(msgType, "setSubSteps") == 0) {
                            if (doc.containsKey("value")) {
                                robotController.setSubSteps(doc["value"].as<uint8_t>());
                            }
                        }
                        else if (strcmp(msgType, "setTimingProfile") == 0) {
                            if (doc.containsKey("value")) {
                                robotController.setTimingProfile((TimingProfile)doc["value"].as<int>());
                            }
                        }
                        else if (strcmp(msgType, "setSwingMul") == 0) {
                            if (doc.containsKey("value")) {
                                robotController.setSwingMultiplier(doc["value"].as<float>());
                            }
                        }
                        else if (strcmp(msgType, "setStanceMul") == 0) {
                            if (doc.containsKey("value")) {
                                robotController.setStanceMultiplier(doc["value"].as<float>());
                            }
                        }
                        else if (strcmp(msgType, "setRamp") == 0) {
                            bool enabled = doc["enabled"] | false;
                            uint8_t cycles = doc["cycles"] | 3;
                            robotController.enableRamp(enabled, cycles);
                        }
                        
                        // ========== v3 Servo-Kalibrierungs-Commands ==========
                        else if (strcmp(msgType, "setServoOffset") == 0) {
                            if (!robotController.isCalibrationLocked()) {
                                uint8_t servo = doc["servo"] | 0;
                                int offset = doc["offset"] | 0;
                                ServoCalibration::setOffset(servo, offset);
                                broadcastCalibState();
                            }
                        }
                        else if (strcmp(msgType, "setServoLimits") == 0) {
                            if (!robotController.isCalibrationLocked()) {
                                uint8_t servo = doc["servo"] | 0;
                                int minAngle = doc["min"] | 20;
                                int maxAngle = doc["max"] | 160;
                                int center = doc["center"] | 90;
                                ServoCalibration::setLimits(servo, minAngle, maxAngle, center);
                                sendServoLimits(client);
                            }
                        }
                        else if (strcmp(msgType, "setServoCalib") == 0) {
                            if (!robotController.isCalibrationLocked()) {
                                uint8_t servo = doc["servo"] | 0;
                                int offset = doc["offset"] | 0;
                                int minAngle = doc["min"] | 20;
                                int maxAngle = doc["max"] | 160;
                                int center = doc["center"] | 90;
                                
                                ServoCalibration::setOffset(servo, offset);
                                ServoCalibration::setLimits(servo, minAngle, maxAngle, center);
                                broadcastCalibState();
                            }
                        }
                        else if (strcmp(msgType, "saveCalib") == 0) {
                            ServoCalibration::save();
                            GaitRuntime::saveConfig();
                            Serial.println(F("[WS] Calibration saved"));
                        }
                        else if (strcmp(msgType, "loadCalib") == 0) {
                            ServoCalibration::load();
                            GaitRuntime::loadConfig();
                            broadcastCalibState();
                            broadcastWalkParams();
                            Serial.println(F("[WS] Calibration loaded"));
                        }
                        else if (strcmp(msgType, "setCalibLock") == 0) {
                            bool locked = doc["locked"] | true;
                            robotController.setCalibrationLocked(locked);
                            broadcastCalibState();
                        }
                        else if (strcmp(msgType, "getCalibState") == 0) {
                            sendCalibState(client);
                        }
                        else if (strcmp(msgType, "getWalkParams") == 0) {
                            sendWalkParams(client);
                        }
                        else if (strcmp(msgType, "getServoLimits") == 0) {
                            sendServoLimits(client);
                        }
                        else if (strcmp(msgType, "getServoCalib") == 0) {
                            sendAllServoCalib(client);
                        }
                        else if (strcmp(msgType, "shutdown") == 0) {
                            performShutdown();
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

// =============================================================================
// Broadcast-Funktionen
// =============================================================================
void broadcastTerrainStatus() {
    JsonDocument doc;
    doc["type"] = "terrain";
    doc["mode"] = getTerrainModeName();
    doc["modeId"] = (int)getTerrainMode();
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void broadcastCalibState() {
    JsonDocument doc;
    doc["type"] = "calibState";
    doc["locked"] = robotController.isCalibrationLocked();
    
    JsonArray offsets = doc["offsets"].to<JsonArray>();
    JsonArray mins = doc["mins"].to<JsonArray>();
    JsonArray maxs = doc["maxs"].to<JsonArray>();
    JsonArray centers = doc["centers"].to<JsonArray>();
    
    for (int i = 0; i < 8; i++) {
        offsets.add(ServoCalibration::getOffset(i));
        mins.add(ServoCalibration::getMinAngle(i));
        maxs.add(ServoCalibration::getMaxAngle(i));
        centers.add(ServoCalibration::getCenterAngle(i));
    }
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void broadcastWalkParams() {
    JsonDocument doc;
    doc["type"] = "walkParams";
    
    const WalkParams& p = robotController.getWalkParams();
    doc["stride"] = p.stride;
    doc["subSteps"] = p.subSteps;
    doc["profile"] = (int)p.profile;
    doc["swingMul"] = p.swingMul;
    doc["stanceMul"] = p.stanceMul;
    doc["rampEnabled"] = p.rampEnabled;
    doc["rampCycles"] = p.rampCycles;
    
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

void sendCalibState(AsyncWebSocketClient *client) {
    JsonDocument doc;
    doc["type"] = "calibState";
    doc["locked"] = robotController.isCalibrationLocked();
    
    JsonArray offsets = doc["offsets"].to<JsonArray>();
    for (int i = 0; i < 8; i++) {
        offsets.add(ServoCalibration::getOffset(i));
    }
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void sendWalkParams(AsyncWebSocketClient *client) {
    JsonDocument doc;
    doc["type"] = "walkParams";
    
    const WalkParams& p = robotController.getWalkParams();
    doc["stride"] = p.stride;
    doc["subSteps"] = p.subSteps;
    doc["profile"] = (int)p.profile;
    doc["swingMul"] = p.swingMul;
    doc["stanceMul"] = p.stanceMul;
    doc["rampEnabled"] = p.rampEnabled;
    doc["rampCycles"] = p.rampCycles;
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void sendServoLimits(AsyncWebSocketClient *client) {
    JsonDocument doc;
    doc["type"] = "servoLimits";
    
    JsonArray mins = doc["mins"].to<JsonArray>();
    JsonArray maxs = doc["maxs"].to<JsonArray>();
    JsonArray centers = doc["centers"].to<JsonArray>();
    
    for (int i = 0; i < 8; i++) {
        mins.add(ServoCalibration::getMinAngle(i));
        maxs.add(ServoCalibration::getMaxAngle(i));
        centers.add(ServoCalibration::getCenterAngle(i));
    }
    
    String output;
    serializeJson(doc, output);
    client->text(output);
}

void sendAllServoCalib(AsyncWebSocketClient *client) {
    JsonDocument doc;
    doc["type"] = "allServoCalib";
    
    JsonArray servos = doc["servos"].to<JsonArray>();
    for (int i = 0; i < 8; i++) {
        JsonObject servo = servos.add<JsonObject>();
        servo["servo"] = i;
        servo["offset"] = ServoCalibration::getOffset(i);
        servo["min"] = ServoCalibration::getMinAngle(i);
        servo["max"] = ServoCalibration::getMaxAngle(i);
        servo["center"] = ServoCalibration::getCenterAngle(i);
    }
    
    String output;
    serializeJson(doc, output);
    client->text(output);
    Serial.println(F("[WS] Sent allServoCalib"));
}

// =============================================================================
// API Routes
// =============================================================================
void setupApiRoutes() {
    webServer.on("/api/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        doc["status"] = "ok";
        doc["version"] = "v3";
        doc["uptime"] = millis();
        doc["clients"] = ws.count();
        doc["moving"] = robotController.isMoving();
        
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
            
            robotController.queueCommand(robotController.parseCommand(name));
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    webServer.on("/api/stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        robotController.forceStop();
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    });
    
    webServer.on("/api/walkparams", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        const WalkParams& p = robotController.getWalkParams();
        doc["stride"] = p.stride;
        doc["subSteps"] = p.subSteps;
        doc["profile"] = (int)p.profile;
        doc["swingMul"] = p.swingMul;
        doc["stanceMul"] = p.stanceMul;
        doc["rampEnabled"] = p.rampEnabled;
        doc["rampCycles"] = p.rampCycles;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
}

// =============================================================================
// Static File Serving
// =============================================================================
void setupStaticFileServing() {
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (LittleFS.exists("/index.html.gz")) {
            AsyncWebServerResponse *response = request->beginResponse(LittleFS, "/index.html.gz", "text/html");
            response->addHeader("Content-Encoding", "gzip");
            request->send(response);
        } else if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(200, "text/html", "<h1>Spider Bot v3</h1><p>Upload web files to LittleFS</p>");
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

        request->send(404, "text/plain", "Not found");
    });
}

// =============================================================================
// Shutdown
// =============================================================================
void performShutdown() {
    Serial.println(F("\n========== SHUTDOWN =========="));
    
    JsonDocument doc;
    doc["type"] = "shutdown";
    doc["message"] = "System shutting down";
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
    delay(100);
    
    robotController.forceStop();
    delay(200);
    
    sleep();
    delay(500);
    
    ServoCalibration::save();
    GaitRuntime::saveConfig();
    
    ws.closeAll();
    delay(100);
    
    webServer.end();
    
    WiFi.disconnect(true);
    delay(100);
    
    Serial.println(F("[Shutdown] Complete. Safe to power off."));
    Serial.flush();
    
    ESP.wdtDisable();
    *((volatile uint32_t*) 0x60000900) &= ~(1);
    
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    
    while (true) {
        delay(1000);
    }
}

// =============================================================================
// Setup
// =============================================================================
void setupWebServer() {
    ws.onEvent(handleWebSocketV3);
    webServer.addHandler(&ws);

    setupApiRoutes();
    setupStaticFileServing();

    webServer.begin();
    Serial.println(F("[Web] Server v3 started on port 80"));
}
