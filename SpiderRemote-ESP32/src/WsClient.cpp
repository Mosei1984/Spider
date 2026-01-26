#include "WsClient.h"

WsClient* WsClient::instance = nullptr;

void WsClient::begin(const char* host, uint16_t port, const char* path, uint32_t minIntervalMs_) {
  instance = this;
  minIntervalMs = minIntervalMs_;
  wsHost = host;
  wsPort = port;
  wsPath = path;

  ws.begin(host, port, path);
  ws.onEvent([](WStype_t t, uint8_t* p, size_t l){ WsClient::onEventThunk(t,p,l); });
  ws.setReconnectInterval(2000);
}

void WsClient::reconnect() {
  ws.disconnect();
  if (wsHost && wsPath) {
    ws.begin(wsHost, wsPort, wsPath);
  }
}

void WsClient::loop() { ws.loop(); }
bool WsClient::connected() { return ws.isConnected(); }

bool WsClient::rateOk() {
  if (!ws.isConnected()) return false;
  uint32_t now = millis();
  if (now - lastSendMs < minIntervalMs) return false;
  lastSendMs = now;
  return true;
}

void WsClient::sendRaw(const char* json) {
  if (!json) return;
  if (!rateOk()) return;
  ws.sendTXT(json);
}

void WsClient::sendImmediate(const char* json) {
  if (!json || !ws.isConnected()) return;
  ws.sendTXT(json);
  delay(10);  // Small delay between rapid sends
}

void WsClient::sendMoveStart(const char* name) {
  if (!name) return;
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"type\":\"moveStart\",\"name\":\"%s\"}", name);
  sendRaw(buf);
}

void WsClient::sendMoveStop() {
  sendRaw("{\"type\":\"moveStop\"}");
}

void WsClient::sendStop() {
  sendRaw("{\"type\":\"stop\"}");
}

void WsClient::sendSetSpeed(int speed) {
  if (speed < 0) speed = 0;
  char buf[48];
  snprintf(buf, sizeof(buf), "{\"type\":\"setSpeed\",\"speed\":%d}", speed);
  sendRaw(buf);
}

void WsClient::sendCmd(const char* name) {
  if (!name) return;
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"type\":\"cmd\",\"name\":\"%s\"}", name);
  sendImmediate(buf);
}

void WsClient::sendTerrain(const char* mode) {
  if (!mode) return;
  char buf[96];
  snprintf(buf, sizeof(buf), "{\"type\":\"setTerrain\",\"mode\":\"%s\"}", mode);
  sendImmediate(buf);
}

void WsClient::onEventThunk(WStype_t type, uint8_t* payload, size_t length) {
  if (instance) instance->onEvent(type, payload, length);
}

void WsClient::onEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      Serial.println("[WS] Connected");
      break;
    case WStype_DISCONNECTED:
      Serial.println("[WS] Disconnected");
      break;
    case WStype_TEXT:
      if (textCallback && payload && length > 0) {
        textCallback((const char*)payload);
      }
      break;
    case WStype_ERROR:
      Serial.println("[WS] Error");
      break;
    default:
      break;
  }
}
