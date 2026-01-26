#pragma once
#include <Arduino.h>
#include <WebSocketsClient.h>

class WsClient {
public:
  void begin(const char* host, uint16_t port, const char* path, uint32_t minIntervalMs);
  void loop();
  bool connected();
  void reconnect();

  // 1:1 JSONs aus Spider index.html
  void sendMoveStart(const char* name);
  void sendMoveStop();
  void sendStop();
  void sendSetSpeed(int speed);
  void sendCmd(const char* name);
  void sendTerrain(const char* mode);
  
  void sendImmediate(const char* json);  // Bypass rate limiting
  
  // Callback für eingehende Nachrichten
  typedef void (*TextCallback)(const char* json);
  void setTextCallback(TextCallback cb) { textCallback = cb; }

private:
  TextCallback textCallback = nullptr;
  WebSocketsClient ws;
  uint32_t minIntervalMs = 50;
  uint32_t lastSendMs = 0;
  
  const char* wsHost = nullptr;
  uint16_t wsPort = 80;
  const char* wsPath = nullptr;

  bool rateOk();
  void sendRaw(const char* json);

  static void onEventThunk(WStype_t type, uint8_t* payload, size_t length);
  void onEvent(WStype_t type, uint8_t* payload, size_t length);
  static WsClient* instance;
};
