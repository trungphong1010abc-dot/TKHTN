#include "thingsboard_manager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static ModeCommandCallback modeCommandCallback = nullptr;
static PumpCommandCallback pumpCommandCallback = nullptr;
static SystemMode tbCurrentMode = AUTO_MODE;

static unsigned long lastWiFiAttemptMs = 0;
static unsigned long lastMqttAttemptMs = 0;

static String getRpcRequestId(const char* topic) {
  const String topicString(topic);
  const int slashIndex = topicString.lastIndexOf('/');
  if (slashIndex < 0) {
    return "";
  }

  return topicString.substring(slashIndex + 1);
}

static String jsonVariantToString(JsonVariant value) {
  if (value.is<const char*>()) {
    return String(value.as<const char*>());
  }

  if (value.is<String>()) {
    return value.as<String>();
  }

  if (value.is<bool>()) {
    return value.as<bool>() ? "ON" : "OFF";
  }

  if (value.is<int>()) {
    return String(value.as<int>());
  }

  if (value.is<float>()) {
    return String(value.as<float>());
  }

  return "";
}

static String getRpcParamString(JsonVariant params) {
  if (params.is<JsonObject>()) {
    if (params["value"]) {
      return jsonVariantToString(params["value"]);
    }
    if (params["mode"]) {
      return jsonVariantToString(params["mode"]);
    }
    if (params["state"]) {
      return jsonVariantToString(params["state"]);
    }
    if (params["enabled"]) {
      return jsonVariantToString(params["enabled"]);
    }
    if (params["pump"]) {
      return jsonVariantToString(params["pump"]);
    }
  }

  return jsonVariantToString(params);
}

static void publishRpcResponse(const String& requestId, bool success, const char* message) {
  if (requestId.length() == 0 || !mqttClient.connected()) {
    return;
  }

  StaticJsonDocument<160> response;
  response["success"] = success;
  response["message"] = message;

  char payload[160];
  const size_t length = serializeJson(response, payload, sizeof(payload));

  String topic = TB_RPC_RESPONSE_TOPIC_PREFIX + requestId;
  mqttClient.publish(topic.c_str(), payload, length);
}

static void handleSetMode(const String& requestId, const String& params) {
  SystemMode requestedMode = AUTO_MODE;

  if (!parseMode(params, requestedMode)) {
    publishRpcResponse(requestId, false, "Invalid mode");
    return;
  }

  if (modeCommandCallback != nullptr) {
    modeCommandCallback(requestedMode);
  }

  tbCurrentMode = requestedMode;
  publishRpcResponse(requestId, true, "Mode updated");
}

static void handleSetPump(const String& requestId, const String& params) {
  if (tbCurrentMode == AUTO_MODE) {
    publishRpcResponse(requestId, false, "Ignored in AUTO mode");
    Serial.println(F("[RPC] setPump ignored in AUTO mode"));
    return;
  }

  const bool turnOn = params.equalsIgnoreCase("ON") ||
                      params.equalsIgnoreCase("TRUE") ||
                      params == "1";

  const bool turnOff = params.equalsIgnoreCase("OFF") ||
                       params.equalsIgnoreCase("FALSE") ||
                       params == "0";

  if (!turnOn && !turnOff) {
    publishRpcResponse(requestId, false, "Invalid pump state");
    return;
  }

  if (pumpCommandCallback != nullptr) {
    pumpCommandCallback(turnOn);
  }

  publishRpcResponse(requestId, true, turnOn ? "Pump ON" : "Pump OFF");
}

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<256> document;
  const DeserializationError error = deserializeJson(document, payload, length);
  const String requestId = getRpcRequestId(topic);

  if (error) {
    publishRpcResponse(requestId, false, "Invalid JSON");
    return;
  }

  const String method = jsonVariantToString(document["method"]);
  const String params = getRpcParamString(document["params"]);

  Serial.print(F("[RPC] Method: "));
  Serial.print(method);
  Serial.print(F(", Params: "));
  Serial.println(params);

  if (method == "setMode") {
    handleSetMode(requestId, params);
  } else if (method == "setPump" || method == "setWater") {
    handleSetPump(requestId, params);
  } else {
    publishRpcResponse(requestId, false, "Unknown method");
  }
}

void thingsBoardBegin() {
  WiFi.mode(WIFI_STA);
  mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(768);
}

void thingsBoardSetCommandCallbacks(ModeCommandCallback modeCallback,
                                    PumpCommandCallback pumpCallback) {
  modeCommandCallback = modeCallback;
  pumpCommandCallback = pumpCallback;
}

void thingsBoardSetCurrentMode(SystemMode mode) {
  tbCurrentMode = mode;
}

bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  const unsigned long nowMs = millis();
  if (lastWiFiAttemptMs != 0 && nowMs - lastWiFiAttemptMs < WIFI_RECONNECT_INTERVAL_MS) {
    return false;
  }

  lastWiFiAttemptMs = nowMs;

  Serial.print(F("[WiFi] Connecting to "));
  Serial.println(WIFI_SSID);

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 5000UL) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("[WiFi] Connected. IP: "));
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println(F("[WiFi] Connection failed"));
  return false;
}

static bool connectMqtt() {
  if (mqttClient.connected()) {
    return true;
  }

  if (!connectWiFi()) {
    return false;
  }

  const unsigned long nowMs = millis();
  if (lastMqttAttemptMs != 0 && nowMs - lastMqttAttemptMs < MQTT_RECONNECT_INTERVAL_MS) {
    return false;
  }

  lastMqttAttemptMs = nowMs;

  const String clientId = "esp32-irrigation-" + String(static_cast<uint32_t>(ESP.getEfuseMac()), HEX);

  Serial.print(F("[MQTT] Connecting to ThingsBoard... "));
  if (mqttClient.connect(clientId.c_str(), THINGSBOARD_ACCESS_TOKEN, nullptr)) {
    Serial.println(F("connected"));
    mqttClient.subscribe(TB_RPC_REQUEST_TOPIC);
    return true;
  }

  Serial.print(F("failed, rc="));
  Serial.println(mqttClient.state());
  return false;
}

void thingsBoardLoop() {
  connectWiFi();

  if (connectMqtt()) {
    mqttClient.loop();
  }
}

bool thingsBoardConnected() {
  return mqttClient.connected();
}

bool publishTelemetry(float temperature,
                      float humidity,
                      float soilMoisture,
                      float tmax,
                      float tmin,
                      float gdd,
                      float cgdd,
                      int stage,
                      bool pumpState,
                      SystemMode mode,
                      const WateringDecision& wateringDecision) {
  if (!connectMqtt()) {
    return false;
  }

  StaticJsonDocument<768> telemetry;
  telemetry["temperature"] = isnan(temperature) ? 0.0f : temperature;
  telemetry["humidity"] = isnan(humidity) ? 0.0f : humidity;
  telemetry["soilMoisture"] = soilMoisture;
  telemetry["tmax"] = isnan(tmax) ? 0.0f : tmax;
  telemetry["tmin"] = isnan(tmin) ? 0.0f : tmin;
  telemetry["gdd"] = gdd;
  telemetry["cgdd"] = cgdd;
  telemetry["stage"] = stage;
  telemetry["pumpState"] = pumpState;
  telemetry["pumpStateText"] = pumpState ? "ON" : "OFF";
  telemetry["mode"] = modeToString(mode);
  telemetry["autoMode"] = mode == AUTO_MODE;
  telemetry["manualMode"] = mode == MANUAL_MODE;
  telemetry["soilStatus"] = wateringDecision.soilStatus;
  telemetry["floodWarning"] = wateringDecision.floodWarning;
  telemetry["pumpTimeSec"] = wateringDecision.pumpTimeSec;
  telemetry["shouldWater"] = wateringDecision.shouldWater;

  char payload[768];
  const size_t length = serializeJson(telemetry, payload, sizeof(payload));

  const bool ok = mqttClient.publish(TB_TELEMETRY_TOPIC, payload, length);
  if (!ok) {
    Serial.println(F("[MQTT] Telemetry publish failed"));
  }

  return ok;
}
