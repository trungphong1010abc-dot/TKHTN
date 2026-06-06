#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <time.h>
#include "config.h"
#include "gdd_manager.h"
#include "pump_controller.h"
#include "sensor_manager.h"
#include "thingsboard_manager.h"

static SensorData latestSensorData{};
static WateringDecision latestWateringDecision{
  0,
  "Unknown",
  false,
  false
};
static Preferences preferences;

static float completedCgdd = 0.0f;
static float currentDailyGdd = 0.0f;
static float dayTmax = NAN;
static float dayTmin = NAN;
static bool hasDayTemperatureSample = false;
static int currentStage = 1;
static int activeDayKey = 0;
static bool timeConfigured = false;
static bool timeSynchronized = false;

static unsigned long lastSensorReadMs = 0;
static unsigned long lastTelemetryMs = 0;
static unsigned long lastSerialPrintMs = 0;
static unsigned long lastTimeSyncCheckMs = 0;
static unsigned long lastDailyStateSaveMs = 0;

static int makeDayKey(const tm& timeInfo) {
  return ((timeInfo.tm_year + 1900) * 10000) +
         ((timeInfo.tm_mon + 1) * 100) +
         timeInfo.tm_mday;
}

static bool readLocalTime(tm& timeInfo) {
  return getLocalTime(&timeInfo, 50);
}

static void configureTimeIfNeeded() {
  if (timeConfigured || WiFi.status() != WL_CONNECTED) {
    return;
  }

  configTime(GMT_OFFSET_SEC,
             DAYLIGHT_OFFSET_SEC,
             NTP_SERVER_PRIMARY,
             NTP_SERVER_SECONDARY,
             NTP_SERVER_TERTIARY);
  timeConfigured = true;
  Serial.println(F("[TIME] NTP configured for UTC+7"));
}

static void maintainTimeSync(unsigned long nowMs) {
  if (nowMs - lastTimeSyncCheckMs < TIME_SYNC_CHECK_INTERVAL_MS) {
    return;
  }

  lastTimeSyncCheckMs = nowMs;
  configureTimeIfNeeded();

  tm timeInfo{};
  if (!readLocalTime(timeInfo)) {
    timeSynchronized = false;
    Serial.println(F("[TIME] Waiting for NTP time"));
    return;
  }

  if (!timeSynchronized) {
    timeSynchronized = true;
    Serial.print(F("[TIME] Synced. Local date key = "));
    Serial.println(makeDayKey(timeInfo));
  }
}

static void saveDailyState(bool force) {
  const unsigned long nowMs = millis();
  if (!force && nowMs - lastDailyStateSaveMs < DAILY_STATE_SAVE_INTERVAL_MS) {
    return;
  }

  preferences.putInt("dayKey", activeDayKey);
  preferences.putBool("hasTemp", hasDayTemperatureSample);
  preferences.putFloat("tmax", dayTmax);
  preferences.putFloat("tmin", dayTmin);
  lastDailyStateSaveMs = nowMs;
}

static void resetDailyTemperatureState(int newDayKey) {
  activeDayKey = newDayKey;
  dayTmax = NAN;
  dayTmin = NAN;
  currentDailyGdd = 0.0f;
  hasDayTemperatureSample = false;
  saveDailyState(true);
}

static void loadGddState() {
  completedCgdd = preferences.getFloat("cgdd", 0.0f);
  activeDayKey = preferences.getInt("dayKey", 0);
  hasDayTemperatureSample = preferences.getBool("hasTemp", false);
  dayTmax = preferences.getFloat("tmax", NAN);
  dayTmin = preferences.getFloat("tmin", NAN);

  if (!hasDayTemperatureSample || isnan(dayTmax) || isnan(dayTmin)) {
    dayTmax = NAN;
    dayTmin = NAN;
    currentDailyGdd = 0.0f;
    hasDayTemperatureSample = false;
  } else {
    currentDailyGdd = calculateGDD(dayTmax, dayTmin);
  }

  currentStage = determineStage(completedCgdd);
}

static void onModeCommand(SystemMode mode) {
  setCurrentMode(mode);
  Serial.print(F("[RPC] Mode changed to "));
  Serial.println(modeToString(mode));
}

static void onPumpCommand(bool turnOn) {
  if (getCurrentMode() != MANUAL_MODE) {
    Serial.println(F("[RPC] setPump ignored because current mode is AUTO"));
    return;
  }

  if (turnOn) {
    startPump();
  } else {
    stopPump();
  }
}

static void updateDailyTemperatureRange(float temperature) {
  if (isnan(dayTmax) || temperature > dayTmax) {
    dayTmax = temperature;
  }

  if (isnan(dayTmin) || temperature < dayTmin) {
    dayTmin = temperature;
  }

  hasDayTemperatureSample = true;
  currentDailyGdd = calculateGDD(dayTmax, dayTmin);
  saveDailyState(false);
}

static void closeDayIfDateChanged() {
  tm timeInfo{};
  if (!readLocalTime(timeInfo)) {
    return;
  }

  const int currentDayKey = makeDayKey(timeInfo);
  if (activeDayKey == 0) {
    resetDailyTemperatureState(currentDayKey);
    return;
  }

  if (currentDayKey == activeDayKey) {
    return;
  }

  if (!hasDayTemperatureSample) {
    resetDailyTemperatureState(currentDayKey);
    return;
  }

  const float finalDailyGdd = calculateGDD(dayTmax, dayTmin);
  completedCgdd += finalDailyGdd;
  preferences.putFloat("cgdd", completedCgdd);

  currentStage = determineStage(completedCgdd);

  Serial.print(F("[GDD] End of day. Daily GDD = "));
  Serial.print(finalDailyGdd, 2);
  Serial.print(F(", CGDD = "));
  Serial.println(completedCgdd, 2);

  resetDailyTemperatureState(currentDayKey);
}

static void printWateringDecision(const char* prefix,
                                  const WateringDecision& decision) {
  Serial.print(prefix);
  Serial.print(F(" Stage="));
  Serial.print(currentStage);
  Serial.print(F(", Soil="));
  Serial.print(latestSensorData.soilPercent, 1);
  Serial.print(F("%, SoilStatus="));
  Serial.print(decision.soilStatus);
  Serial.print(F(", Temp="));
  Serial.print(latestSensorData.temperature, 1);
  Serial.print(F("C, RH="));
  Serial.print(latestSensorData.humidity, 1);
  Serial.print(F("%, FloodWarning="));
  Serial.print(decision.floodWarning ? F("YES") : F("NO"));
  Serial.print(F(", PumpTime="));
  Serial.print(decision.pumpTimeSec);
  Serial.println(F("s"));
}

static void handleAutomaticWatering() {
  if (getCurrentMode() != AUTO_MODE) {
    printWateringDecision("[AUTO] Manual mode", latestWateringDecision);
    return;
  }

  if (isPumpRunning()) {
    printWateringDecision("[AUTO] Pump running", latestWateringDecision);
    return;
  }

  if (latestSensorData.errorFlag) {
    latestWateringDecision = {
      0,
      latestSensorData.soilValid ? "Unknown" : "Sensor Error",
      false,
      false
    };
    printWateringDecision("[AUTO] Sensor error", latestWateringDecision);
    return;
  }

  latestWateringDecision = calculateWateringDecision(latestSensorData.soilPercent,
                                                     latestSensorData.temperature,
                                                     latestSensorData.humidity,
                                                     currentStage);
  printWateringDecision("[AUTO]", latestWateringDecision);

  if (!latestWateringDecision.shouldWater) {
    return;
  }

  if (!pumpCanStart(AUTO_WATER_COOLDOWN_MS)) {
    printWateringDecision("[AUTO] Cooldown active", latestWateringDecision);
    return;
  }

  Serial.print(F("[AUTO] Starting pump for "));
  Serial.print(latestWateringDecision.pumpTimeSec);
  Serial.println(F(" seconds"));
  startPump(static_cast<unsigned long>(latestWateringDecision.pumpTimeSec));
}

static void printStatus() {
  Serial.println(F("========== SYSTEM STATUS =========="));
  Serial.print(F("Temperature: "));
  Serial.print(latestSensorData.temperature, 1);
  Serial.println(F(" C"));

  Serial.print(F("Humidity: "));
  Serial.print(latestSensorData.humidity, 1);
  Serial.println(F(" %RH"));

  Serial.print(F("Soil ADC: "));
  Serial.println(latestSensorData.soilAdcRaw);

  Serial.print(F("Soil ADC filtered: "));
  Serial.println(latestSensorData.soilAdcFiltered);

  Serial.print(F("SoilPercent: "));
  Serial.print(latestSensorData.soilPercent, 1);
  Serial.println(F(" %"));

  Serial.print(F("DHT22_status: "));
  Serial.println(sensorStatusText(latestSensorData.dhtValid));

  Serial.print(F("Soil_status: "));
  Serial.println(sensorStatusText(latestSensorData.soilValid));

  Serial.print(F("SoilStatus: "));
  Serial.println(latestWateringDecision.soilStatus);

  Serial.print(F("FloodWarning: "));
  Serial.println(latestWateringDecision.floodWarning ? F("YES") : F("NO"));

  Serial.print(F("PumpTimeSec: "));
  Serial.println(latestWateringDecision.pumpTimeSec);

  Serial.print(F("GDD today estimate: "));
  Serial.println(currentDailyGdd, 2);

  Serial.print(F("CGDD completed: "));
  Serial.println(completedCgdd, 2);

  Serial.print(F("ActiveDayKey: "));
  Serial.println(activeDayKey);

  Serial.print(F("TimeSync: "));
  Serial.println(timeSynchronized ? F("SYNCED") : F("WAITING"));

  Serial.print(F("Stage: "));
  Serial.print(currentStage);
  Serial.print(F(" - "));
  Serial.println(stageName(currentStage));

  Serial.print(F("Mode: "));
  Serial.println(modeToString(getCurrentMode()));

  Serial.print(F("PumpState: "));
  Serial.println(isPumpRunning() ? F("ON") : F("OFF"));

  Serial.print(F("WiFi: "));
  Serial.println(WiFi.status() == WL_CONNECTED ? F("CONNECTED") : F("DISCONNECTED"));

  Serial.print(F("ThingsBoard: "));
  Serial.println(thingsBoardConnected() ? F("CONNECTED") : F("DISCONNECTED"));
  Serial.println(F("==================================="));
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println(F("\nESP32 GDD/CGDD Automatic Irrigation System"));

  pumpBegin();
  sensorBegin();
  setCurrentMode(AUTO_MODE);

  preferences.begin("gdd_state", false);
  loadGddState();

  Serial.print(F("[GDD] Loaded CGDD = "));
  Serial.println(completedCgdd, 2);

  thingsBoardBegin();
  thingsBoardSetCommandCallbacks(onModeCommand, onPumpCommand);

  const unsigned long nowMs = millis();
  lastSensorReadMs = nowMs - SENSOR_READ_INTERVAL_MS;
  lastTelemetryMs = nowMs - TELEMETRY_INTERVAL_MS;
  lastSerialPrintMs = nowMs - SERIAL_PRINT_INTERVAL_MS;
  lastTimeSyncCheckMs = nowMs - TIME_SYNC_CHECK_INTERVAL_MS;
  lastDailyStateSaveMs = nowMs;

  connectWiFi();
  configureTimeIfNeeded();
}

void loop() {
  const unsigned long nowMs = millis();

  thingsBoardSetCurrentMode(getCurrentMode());
  thingsBoardLoop();
  maintainTimeSync(nowMs);
  pumpUpdate();

  if (nowMs - lastSensorReadMs >= SENSOR_READ_INTERVAL_MS) {
    latestSensorData = readSensors();
    lastSensorReadMs = nowMs;

    if (latestSensorData.dhtValid) {
      closeDayIfDateChanged();
      updateDailyTemperatureRange(latestSensorData.temperature);
    }

    handleAutomaticWatering();
  }

  if (nowMs - lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
    publishTelemetry(latestSensorData.temperature,
                     latestSensorData.humidity,
                     latestSensorData.soilPercent,
                     dayTmax,
                     dayTmin,
                     currentDailyGdd,
                     completedCgdd,
                     currentStage,
                     isPumpRunning(),
                     getCurrentMode(),
                     latestWateringDecision);
    lastTelemetryMs = nowMs;
  }

  if (nowMs - lastSerialPrintMs >= SERIAL_PRINT_INTERVAL_MS) {
    printStatus();
    lastSerialPrintMs = nowMs;
  }
}
