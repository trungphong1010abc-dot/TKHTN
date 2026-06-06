#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------- Hardware pins ----------------
constexpr uint8_t DHT_PIN = 27;
constexpr uint8_t DHT_TYPE = 22;
constexpr uint8_t SOIL_ADC_PIN = 34;
constexpr uint8_t RELAY_PIN = 26;

// Most 1-channel relay modules are active LOW. Change to HIGH if your relay is active HIGH.
constexpr uint8_t RELAY_ACTIVE_LEVEL = LOW;
constexpr uint8_t RELAY_INACTIVE_LEVEL = (RELAY_ACTIVE_LEVEL == LOW) ? HIGH : LOW;

// ---------------- Soil calibration ----------------
// Calibrate these values with Serial Monitor:
// ADC_DRY: raw ADC value when the sensor is in dry soil/air.
// ADC_WET: raw ADC value when the sensor is in very wet soil/water.
constexpr int ADC_DRY = 3400;
constexpr int ADC_WET = 1200;
constexpr int ADC_MIN_VALID = 100;
constexpr int ADC_MAX_VALID = 4000;
constexpr uint8_t SOIL_SAMPLE_COUNT = 11;
constexpr uint8_t SOIL_DISCARD_COUNT = 3;
constexpr uint8_t SOIL_MAX_ERROR_COUNT = 3;

// ---------------- DHT22 validation/filtering ----------------
constexpr float TEMP_OFFSET_C = 0.0f;
constexpr float RH_OFFSET_PERCENT = 0.0f;
constexpr float DHT_TEMP_MIN_C = -10.0f;
constexpr float DHT_TEMP_MAX_C = 50.0f;
constexpr float DHT_RH_MIN_PERCENT = 0.0f;
constexpr float DHT_RH_MAX_PERCENT = 100.0f;
constexpr float DHT_MAX_TEMP_STEP_C = 2.0f;
constexpr float DHT_MAX_RH_STEP_PERCENT = 5.0f;
constexpr uint8_t DHT_MOVING_AVERAGE_WINDOW = 5;
constexpr uint8_t DHT_MAX_ERROR_COUNT = 3;

// ---------------- GDD / crop stages ----------------
constexpr float TBASE_C = 10.0f;

constexpr float STAGE_1_MAX_CGDD = 150.0f;
constexpr float STAGE_2_MAX_CGDD = 400.0f;

// ---------------- Automatic watering ----------------
constexpr int PUMP_TIME_MIN_SEC = 3;
constexpr int PUMP_TIME_MAX_SEC = 30;
constexpr unsigned long AUTO_WATER_COOLDOWN_MS = 15UL * 60UL * 1000UL;

// ---------------- Timing ----------------
constexpr unsigned long SENSOR_READ_INTERVAL_MS = 3000UL;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 10000UL;
constexpr unsigned long SERIAL_PRINT_INTERVAL_MS = 3000UL;
constexpr unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000UL;
constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS = 5000UL;
constexpr unsigned long TIME_SYNC_CHECK_INTERVAL_MS = 15000UL;
constexpr unsigned long DAILY_STATE_SAVE_INTERVAL_MS = 10UL * 60UL * 1000UL;

// ---------------- Real time clock via NTP ----------------
constexpr long GMT_OFFSET_SEC = 7L * 60L * 60L;
constexpr int DAYLIGHT_OFFSET_SEC = 0;
constexpr char NTP_SERVER_PRIMARY[] = "pool.ntp.org";
constexpr char NTP_SERVER_SECONDARY[] = "time.nist.gov";
constexpr char NTP_SERVER_TERTIARY[] = "time.google.com";

// ---------------- WiFi / ThingsBoard ----------------
constexpr char WIFI_SSID[] = "TrungPhong_2.4G";
constexpr char WIFI_PASSWORD[] = "1980abcd";

constexpr char THINGSBOARD_SERVER[] = "mqtt.eu.thingsboard.cloud";
constexpr uint16_t THINGSBOARD_PORT = 1883;
constexpr char THINGSBOARD_ACCESS_TOKEN[] = "RFcNCcd769OQQVrzviCW";

constexpr char TB_TELEMETRY_TOPIC[] = "v1/devices/me/telemetry";
constexpr char TB_RPC_REQUEST_TOPIC[] = "v1/devices/me/rpc/request/+";
constexpr char TB_RPC_RESPONSE_TOPIC_PREFIX[] = "v1/devices/me/rpc/response/";

#endif
