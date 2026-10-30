#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID = "TU_WIFI";
const char* WIFI_PASS = "TU_CLAVE_WIFI";
const char* SERVER_URL = "https://TU-SERVICIO-URL";
const char* API_KEY = "TU_API_KEY";

const int SOIL_PIN = 34;
const unsigned long SEND_INTERVAL = 5 * 60 * 1000; // 5 minutos

unsigned long lastSend = 0;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado");
}

void sendTelemetry() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  int soilRaw = analogRead(SOIL_PIN);
  float soilPct = map(soilRaw, 3200, 1200, 0, 100); // Calibrar según tu sensor

  JsonDocument doc;
  doc["device_id"] = "esp32-01";
  doc["soil_moisture_pct"] = soilPct;
  doc["air_temperature_c"] = 0.0; // Reemplazar por DHT22/BME280
  doc["air_humidity_pct"] = 0.0;
  doc["light_lux"] = 0.0;
  doc["pump_state"] = 0;
  doc["fan_state"] = 0;

  String payload;
  serializeJson(doc, payload);

  WiFiClientSecure client;
  client.setInsecure(); // Solo pruebas

  HTTPClient http;
  http.begin(client, String(SERVER_URL) + "/telemetry");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-API-Key", API_KEY);

  int code = http.POST(payload);
  Serial.printf("POST: %d\n", code);
  http.end();
}

void downloadCsv() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.begin(client, String(SERVER_URL) + "/data.csv");
  http.addHeader("X-API-Key", API_KEY);

  int code = http.GET();
  Serial.printf("GET CSV: %d\n", code);

  if (code == 200) {
    WiFiClient* stream = http.getStreamPtr();
    while (stream->available()) Serial.write(stream->read());
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
}

void loop() {
  if (millis() - lastSend >= SEND_INTERVAL) {
    sendTelemetry();
    lastSend = millis();
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "csv") downloadCsv();
  }
}