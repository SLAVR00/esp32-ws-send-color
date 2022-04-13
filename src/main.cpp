#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ArduinoWebsockets.h>
#include <WiFi.h>

#define PIN_R 33
#define PIN_G 32
#define PIN_B 35
#define PIN_A 34

#define MAP_FROM_MIN 0
#define MAP_FROM_MAX 4095
#define MAP_TO_MIN 0
#define MAP_TO_MAX 255

#define LPF_ALPHA 0.98
int LPF_BUFF[100] = {0}; // :)

const char *SSID = "";
const char *PASSWORD = "";

using namespace websockets;
WebsocketsServer ws_server;
WebsocketsClient ws_client;

void setupWifi();
void setupWebSocket();
byte getVolume(int pin);
uint32_t rgba2hex(byte R, byte G, byte B, byte A);
bool isUpdatedValue(uint32_t rgba);

void setup() {
  Serial.begin(9600);
  setupWifi();
  setupWebSocket();
  delay(100);
}

void loop() {
  if(!ws_client.available())
    return void(ws_client = ws_server.accept());
  // Generates hex color code from analog inputs
  uint32_t rgba_hex = rgba2hex(
    getVolume(PIN_R),
    getVolume(PIN_G),
    getVolume(PIN_B),
    getVolume(PIN_A)
  );
  // Check update
  if (!isUpdatedValue(rgba_hex)) return;
  // Hex formatting
  char hex_str[8];
  sprintf(hex_str, "%08x", rgba_hex);
  // Generate json
  JSONVar json;
  json["colorHex"] = hex_str;
  ws_client.send(JSON.stringify(json));
  // Affects LPF
  delay(1);
}

inline byte getVolume(int pin) {
  // Read Volume
  uint16_t vol = analogRead(pin);
  // Calculate LPF
  LPF_BUFF[pin] =
    LPF_ALPHA * LPF_BUFF[pin]
    + (1 - LPF_ALPHA) * vol;
  // Normalize
  uint16_t cvt = map(
    LPF_BUFF[pin],
    MAP_FROM_MIN,
    MAP_FROM_MAX,
    MAP_TO_MIN,
    MAP_TO_MAX + 5
  );
  // Tail Cut
  return min((uint16_t)255, cvt);
}

inline uint32_t rgba2hex(
  byte R, byte G, byte B, byte A
) {
  return ((R & 0xff) << 24)
      + ((G & 0xff) << 16)
      + ((B & 0xff) << 8)
      + ((A & 0xff) << 0);
}

bool isUpdatedValue(uint32_t rgba) {
  static uint32_t prev = 0;
  bool isUpdated = prev != 0 && prev != rgba;
  prev = rgba;
  return isUpdated;
}

void setupWifi() {
  WiFi.begin(SSID, PASSWORD);
  while (
    WiFi.status() != WL_CONNECTED
  ) delay(500);
  Serial.print("\nConnected:");
  Serial.println(WiFi.localIP());
};

void setupWebSocket() {
  ws_server.listen(80);
  ws_client = ws_server.accept();
};
