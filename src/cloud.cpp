#include "cloud.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>

#include "config.h"

namespace {
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
bool timeConfigured = false;

bool isPlaceholder(const char *value) {
  return value == nullptr || value[0] == '\0' || strncmp(value, "YOUR_", 5) == 0;
}

void connectWiFiIfNeeded() {
  if (WiFi.status() == WL_CONNECTED || isPlaceholder(WIFI_SSID)) {
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

bool connectMqttIfNeeded() {
  if (mqttClient.connected()) {
    return true;
  }
  if (WiFi.status() != WL_CONNECTED || isPlaceholder(THINGSBOARD_TOKEN)) {
    return false;
  }

  const String clientId = String("esp32-irrigation-") + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);
  return mqttClient.connect(clientId.c_str(), THINGSBOARD_TOKEN, nullptr);
}

void syncTimeIfNeeded() {
  if (timeConfigured || WiFi.status() != WL_CONNECTED) {
    return;
  }

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2);
  timeConfigured = true;
}
}  // namespace

void cloudBegin() {
  mqttClient.setServer(THINGSBOARD_HOST, THINGSBOARD_PORT);
  mqttClient.setBufferSize(2048);
  connectWiFiIfNeeded();
}

void cloudLoop() {
  connectWiFiIfNeeded();
  if (WiFi.status() == WL_CONNECTED) {
    syncTimeIfNeeded();
    connectMqttIfNeeded();
    mqttClient.loop();
  }
}

TelemetryPacket_t makeTelemetryPacket(const SensorData_t &SensorData, const ControlData_t &ControlData, const PumpState_t &PumpStateData) {
  TelemetryPacket_t packet;
  packet.SensorData = SensorData;
  packet.ControlData = ControlData;
  packet.PumpStateData = PumpStateData;
  packet.wifi_status = WiFi.status() == WL_CONNECTED;
  packet.cloud_status = mqttClient.connected();
  packet.timestamp = millis();
  return packet;
}

bool publishTelemetryPacket(const TelemetryPacket_t &TelemetryPacket) {
  if (WiFi.status() != WL_CONNECTED || !connectMqttIfNeeded()) {
    return false;
  }

  StaticJsonDocument<1536> doc;
  doc["timestamp"] = TelemetryPacket.timestamp;

  doc["T_air"] = TelemetryPacket.SensorData.dhtData.T_air;
  doc["H_air"] = TelemetryPacket.SensorData.dhtData.H_air;
  doc["DHT_status"] = statusToText(TelemetryPacket.SensorData.dhtData.DHT_status);
  doc["DHT_Error_Flag"] = TelemetryPacket.SensorData.dhtData.DHT_Error_Flag;
  doc["ADC_filtered"] = TelemetryPacket.SensorData.soilData.ADC_filtered;
  doc["H_soil"] = TelemetryPacket.SensorData.soilData.H_soil;
  doc["Soil_status"] = statusToText(TelemetryPacket.SensorData.soilData.Soil_status);
  doc["Soil_Error_Flag"] = TelemetryPacket.SensorData.soilData.Soil_Error_Flag;
  doc["Error_Flag"] = TelemetryPacket.SensorData.Error_Flag;
  doc["sensor_timestamp"] = TelemetryPacket.SensorData.timestamp;

  doc["data_valid"] = TelemetryPacket.ControlData.data_valid;
  doc["T_max"] = TelemetryPacket.ControlData.T_max;
  doc["T_min"] = TelemetryPacket.ControlData.T_min;
  doc["T_avg"] = TelemetryPacket.ControlData.T_avg;
  doc["GDD"] = TelemetryPacket.ControlData.GDD;
  doc["CGDD"] = TelemetryPacket.ControlData.CGDD;
  doc["current_stage"] = TelemetryPacket.ControlData.current_stage;
  doc["soil_state"] = TelemetryPacket.ControlData.soil_state;
  doc["H_threshold"] = TelemetryPacket.ControlData.H_threshold;
  doc["WATER_DURATION_MS"] = TelemetryPacket.ControlData.WATER_DURATION_MS;
  doc["watering_duration_ms"] = TelemetryPacket.ControlData.watering_duration;
  doc["watering_duration"] = TelemetryPacket.ControlData.watering_duration / 1000.0f;
  doc["water_duration"] = TelemetryPacket.ControlData.watering_duration / 1000.0f;
  doc["WATER_DURATION_SEC"] = TelemetryPacket.ControlData.WATER_DURATION_MS / 1000.0f;
  doc["watering_duration_sec"] = TelemetryPacket.ControlData.watering_duration / 1000.0f;
  doc["pump_cmd"] = pumpStateToText(TelemetryPacket.ControlData.pump_cmd);
  doc["control_status"] = controlStatusToText(TelemetryPacket.ControlData.control_status);
  doc["control_timestamp"] = TelemetryPacket.ControlData.timestamp;
  doc["pump_state"] = pumpStateToText(TelemetryPacket.PumpStateData.pump_state);
  doc["pump_state_timestamp"] = TelemetryPacket.PumpStateData.timestamp;
  doc["wifi_status"] = WiFi.status() == WL_CONNECTED;
  doc["cloud_status"] = mqttClient.connected();

  char payload[1536];
  const size_t length = serializeJson(doc, payload, sizeof(payload));
  return mqttClient.publish("v1/devices/me/telemetry", payload, length);
}
