#pragma once

#include <Arduino.h>

// Hardware pins
constexpr uint8_t DHT_PIN = 27;
constexpr uint8_t DHT_TYPE = 22;
constexpr uint8_t SOIL_ADC_PIN = 34;
constexpr uint8_t PUMP_PIN = 26;
// Set this to true when the pump/relay turns ON with HIGH output.
// Set to false when the pump/relay turns ON with LOW output.
constexpr bool PUMP_ACTIVE_HIGH = true;

// Soil sensor calibration.
// Update these two values after measuring your actual capacitive sensor:
// ADC high usually means dry soil, ADC low usually means wet soil.
constexpr uint16_t SOIL_ADC_DRY = 3400;
constexpr uint16_t SOIL_ADC_WET = 1200;
constexpr uint8_t SOIL_ADC_SAMPLES = 12;

// Task periods and delays
constexpr uint32_t SENSOR_PERIOD_MS = 60000UL;
constexpr uint32_t TELEMETRY_PERIOD_MS = 1000UL;
constexpr uint32_t SOIL_SETTLE_DELAY_MS = 1000UL;
constexpr uint32_t ADC_RETRY_DELAY_MS = 400UL;
constexpr uint32_t DHT_STABILIZE_DELAY_MS = 2000UL;
constexpr uint32_t MIN_WATER_INTERVAL = 1800000UL;
constexpr uint32_t WATCHDOG_TIMEOUT_SEC = 30UL;

// GDD/CGDD model
constexpr float T_base = 10.0f;
constexpr float STAGE_2_CGDD = 300.0f;
constexpr float STAGE_3_CGDD = 700.0f;
constexpr long GMT_OFFSET_SEC = 7L * 3600L;
constexpr int DAYLIGHT_OFFSET_SEC = 0;
constexpr char NTP_SERVER_1[] = "pool.ntp.org";
constexpr char NTP_SERVER_2[] = "time.nist.gov";

// WiFi and ThingsBoard MQTT placeholders.
// Fill these before enabling real telemetry.
constexpr char WIFI_SSID[] = "You Tea T1";
constexpr char WIFI_PASSWORD[] = "88888888";
constexpr char THINGSBOARD_HOST[] = "mqtt.eu.thingsboard.cloud";
constexpr uint16_t THINGSBOARD_PORT = 1883;
constexpr char THINGSBOARD_TOKEN[] = "8O6ZRLHpYf3lY8V8lv8s";

// FreeRTOS priority mapping from the design docs.
constexpr UBaseType_t PRIORITY_TASK_ACTUATOR = 5;
constexpr UBaseType_t PRIORITY_TASK_CONTROL = 4;
constexpr UBaseType_t PRIORITY_TASK_SENSOR = 3;
constexpr UBaseType_t PRIORITY_TASK_FEEDBACK = 3;
constexpr UBaseType_t PRIORITY_TASK_CLOUD = 2;
