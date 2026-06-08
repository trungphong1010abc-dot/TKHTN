#include <Arduino.h>
#include <esp_task_wdt.h>

#include "actuator.h"
#include "cloud.h"
#include "config.h"
#include "control.h"
#include "sensing.h"
#include "types.h"

namespace {
QueueHandle_t sensorToControlQueue = nullptr;
QueueHandle_t actuatorCmdQueue = nullptr;
QueueHandle_t actuatorFeedbackQueue = nullptr;
QueueHandle_t controlToCloudQueue = nullptr;
QueueHandle_t cloudTelemetryQueue = nullptr;
SemaphoreHandle_t latestSensorMutex = nullptr;
SemaphoreHandle_t latestPumpStateMutex = nullptr;

SensorData_t latestSensorData;
PumpState_t latestPumpStateData;

void watchdog_reset_routine() {
  esp_task_wdt_reset();
}

void watchdogRegisterCurrentTask() {
  esp_task_wdt_add(nullptr);
  watchdog_reset_routine();
}

void watchdogDelay(uint32_t delay_ms) {
  uint32_t elapsed = 0;
  while (elapsed < delay_ms) {
    const uint32_t chunk = min<uint32_t>(1000UL, delay_ms - elapsed);
    vTaskDelay(pdMS_TO_TICKS(chunk));
    elapsed += chunk;
    watchdog_reset_routine();
  }
}

void updateLatestSensorData(const SensorData_t &SensorData) {
  if (latestSensorMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(latestSensorMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (SensorData.timestamp >= latestSensorData.timestamp) {
      latestSensorData = SensorData;
    }
    xSemaphoreGive(latestSensorMutex);
  }
}

SensorData_t getLatestSensorData() {
  SensorData_t snapshot;
  if (latestSensorMutex != nullptr && xSemaphoreTake(latestSensorMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    snapshot = latestSensorData;
    xSemaphoreGive(latestSensorMutex);
  }
  return snapshot;
}

void updateLatestPumpState(const PumpState_t &PumpStateData) {
  if (latestPumpStateMutex == nullptr) {
    return;
  }

  if (xSemaphoreTake(latestPumpStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    if (PumpStateData.timestamp >= latestPumpStateData.timestamp) {
      latestPumpStateData = PumpStateData;
    }
    xSemaphoreGive(latestPumpStateMutex);
  }
}

PumpState_t getLatestPumpState() {
  PumpState_t snapshot;
  if (latestPumpStateMutex != nullptr && xSemaphoreTake(latestPumpStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    snapshot = latestPumpStateData;
    xSemaphoreGive(latestPumpStateMutex);
  }
  return snapshot;
}

void printSensorData(const char *tag, const SensorData_t &SensorData) {
  Serial.print(tag);
  Serial.print(" DHT=");
  Serial.print(statusToText(SensorData.dhtData.DHT_status));
  Serial.print(" T_air=");
  Serial.print(SensorData.dhtData.T_air);
  Serial.print(" H_air=");
  Serial.print(SensorData.dhtData.H_air);
  Serial.print(" Soil=");
  Serial.print(statusToText(SensorData.soilData.Soil_status));
  Serial.print(" ADC=");
  Serial.print(SensorData.soilData.ADC_filtered);
  Serial.print(" H_soil=");
  Serial.print(SensorData.soilData.H_soil);
  Serial.print(" error=");
  Serial.println(SensorData.Error_Flag ? "YES" : "NO");
}

void printTelemetryData(const TelemetryPacket_t &TelemetryPacket) {
  const SensorData_t &SensorData = TelemetryPacket.SensorData;
  const ControlData_t &ControlData = TelemetryPacket.ControlData;
  const PumpState_t &PumpStateData = TelemetryPacket.PumpStateData;

  Serial.print("[Telemetry]");
  Serial.print(" timestamp=");
  Serial.print(TelemetryPacket.timestamp);
  Serial.print(" wifi_status=");
  Serial.print(TelemetryPacket.wifi_status ? "OK" : "OFF");
  Serial.print(" cloud_status=");
  Serial.print(TelemetryPacket.cloud_status ? "OK" : "OFF");
  Serial.print(" T_air=");
  Serial.print(SensorData.dhtData.T_air);
  Serial.print(" H_air=");
  Serial.print(SensorData.dhtData.H_air);
  Serial.print(" DHT_status=");
  Serial.print(statusToText(SensorData.dhtData.DHT_status));
  Serial.print(" DHT_Error_Flag=");
  Serial.print(SensorData.dhtData.DHT_Error_Flag ? "true" : "false");
  Serial.print(" ADC_filtered=");
  Serial.print(SensorData.soilData.ADC_filtered);
  Serial.print(" H_soil=");
  Serial.print(SensorData.soilData.H_soil);
  Serial.print(" Soil_status=");
  Serial.print(statusToText(SensorData.soilData.Soil_status));
  Serial.print(" Soil_Error_Flag=");
  Serial.print(SensorData.soilData.Soil_Error_Flag ? "true" : "false");
  Serial.print(" Error_Flag=");
  Serial.print(SensorData.Error_Flag ? "true" : "false");
  Serial.print(" GDD=");
  Serial.print(ControlData.GDD);
  Serial.print(" CGDD=");
  Serial.print(ControlData.CGDD);
  Serial.print(" current_stage=");
  Serial.print(ControlData.current_stage);
  Serial.print(" soil_state=");
  Serial.print(ControlData.soil_state);
  Serial.print(" H_threshold=");
  Serial.print(ControlData.H_threshold);
  Serial.print(" WATER_DURATION_MS=");
  Serial.print(ControlData.WATER_DURATION_MS);
  Serial.print(" watering_duration=");
  Serial.print(ControlData.watering_duration);
  Serial.print(" WATER_DURATION_SEC=");
  Serial.print(ControlData.WATER_DURATION_MS / 1000.0f);
  Serial.print(" watering_duration_sec=");
  Serial.print(ControlData.watering_duration / 1000.0f);
  Serial.print(" pump_cmd=");
  Serial.print(pumpStateToText(ControlData.pump_cmd));
  Serial.print(" pump_state=");
  Serial.print(pumpStateToText(PumpStateData.pump_state));
  Serial.print(" control_status=");
  Serial.println(controlStatusToText(ControlData.control_status));
}

void sendSensorToControlAndCloud(const SensorData_t &SensorData) {
  xQueueSend(sensorToControlQueue, &SensorData, pdMS_TO_TICKS(100));
  xQueueSend(cloudTelemetryQueue, &SensorData, pdMS_TO_TICKS(100));
  updateLatestSensorData(SensorData);
}

void sendPumpCmd(PumpState pump_cmd) {
  PumpCmd_t command;
  command.pump_cmd = pump_cmd;
  command.timestamp = millis();
  xQueueSend(actuatorCmdQueue, &command, pdMS_TO_TICKS(100));
}

void Task_Sensor(void *parameter) {
  (void)parameter;
  watchdogRegisterCurrentTask();

  for (;;) {
    watchdog_reset_routine();
    const SensorData_t SensorData = readSensorData();
    printSensorData("[Sensor]", SensorData);
    sendSensorToControlAndCloud(SensorData);
    watchdog_reset_routine();
    watchdogDelay(SENSOR_PERIOD_MS);
  }
}

void Task_Control(void *parameter) {
  (void)parameter;
  watchdogRegisterCurrentTask();
  SensorData_t SensorData;

  for (;;) {
    watchdog_reset_routine();
    if (xQueueReceive(sensorToControlQueue, &SensorData, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;
    }
    watchdog_reset_routine();

    ControlData_t ControlData = processControlData(SensorData);

    if (ControlData.pump_cmd == PumpState::ON && ControlData.WATER_DURATION_MS > 0) {
      sendPumpCmd(PumpState::ON);
      xQueueSend(controlToCloudQueue, &ControlData, pdMS_TO_TICKS(100));

      watchdogDelay(ControlData.WATER_DURATION_MS);
      watchdog_reset_routine();

      sendPumpCmd(PumpState::OFF);
      ControlData.pump_cmd = PumpState::OFF;
      ControlData.control_status = ControlStatus::WATERING_DONE;
      ControlData.timestamp = millis();
      xQueueSend(controlToCloudQueue, &ControlData, pdMS_TO_TICKS(100));
    } else {
      sendPumpCmd(PumpState::OFF);
      xQueueSend(controlToCloudQueue, &ControlData, pdMS_TO_TICKS(100));
    }
  }
}

void Task_Actuator(void *parameter) {
  (void)parameter;
  watchdogRegisterCurrentTask();
  PumpCmd_t command;

  for (;;) {
    watchdog_reset_routine();
    if (xQueueReceive(actuatorCmdQueue, &command, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;
    }
    watchdog_reset_routine();

    const PumpState_t pump_state = applyPumpCmd(command);
    updateLatestPumpState(pump_state);
    xQueueSend(actuatorFeedbackQueue, &pump_state, pdMS_TO_TICKS(100));
  }
}

void Task_Feedback(void *parameter) {
  (void)parameter;
  watchdogRegisterCurrentTask();
  PumpState_t pump_state;

  for (;;) {
    watchdog_reset_routine();
    if (xQueueReceive(actuatorFeedbackQueue, &pump_state, pdMS_TO_TICKS(1000)) != pdTRUE) {
      continue;
    }
    watchdog_reset_routine();

    if (pump_state.pump_state == PumpState::ON) {
      continue;
    }

    watchdogDelay(SOIL_SETTLE_DELAY_MS);
    watchdog_reset_routine();

    const SensorData_t feedbackSensorData = readFeedbackSensorData(getLatestSensorData());
    printSensorData("[Feedback]", feedbackSensorData);
    sendSensorToControlAndCloud(feedbackSensorData);
  }
}

void Task_Cloud(void *parameter) {
  (void)parameter;
  watchdogRegisterCurrentTask();
  SensorData_t SensorData;
  ControlData_t ControlData;
  TickType_t lastTelemetryTime = 0;
  TickType_t lastWaitingLogTime = 0;

  for (;;) {
    watchdog_reset_routine();
    cloudLoop();

    SensorData_t queuedSensorData;
    while (xQueueReceive(cloudTelemetryQueue, &queuedSensorData, 0) == pdTRUE) {
      if (queuedSensorData.timestamp >= SensorData.timestamp) {
        SensorData = queuedSensorData;
      }
    }

    ControlData_t queuedControlData;
    while (xQueueReceive(controlToCloudQueue, &queuedControlData, 0) == pdTRUE) {
      if (queuedControlData.timestamp >= ControlData.timestamp) {
        ControlData = queuedControlData;
      }
    }

    if (SensorData.timestamp == 0) {
      const TickType_t now = xTaskGetTickCount();
      if (now - lastWaitingLogTime >= pdMS_TO_TICKS(1000)) {
        Serial.println("[Cloud] waiting for sensor data");
        lastWaitingLogTime = now;
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    const TickType_t now = xTaskGetTickCount();
    if (lastTelemetryTime != 0 && now - lastTelemetryTime < pdMS_TO_TICKS(TELEMETRY_PERIOD_MS)) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    TelemetryPacket_t TelemetryPacket = makeTelemetryPacket(SensorData, ControlData, getLatestPumpState());
    TelemetryPacket.cloud_status = publishTelemetryPacket(TelemetryPacket);
    printTelemetryData(TelemetryPacket);
    watchdog_reset_routine();
    lastTelemetryTime = now;

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void createQueues() {
  sensorToControlQueue = xQueueCreate(8, sizeof(SensorData_t));
  actuatorCmdQueue = xQueueCreate(8, sizeof(PumpCmd_t));
  actuatorFeedbackQueue = xQueueCreate(8, sizeof(PumpState_t));
  controlToCloudQueue = xQueueCreate(8, sizeof(ControlData_t));
  cloudTelemetryQueue = xQueueCreate(8, sizeof(SensorData_t));
  latestSensorMutex = xSemaphoreCreateMutex();
  latestPumpStateMutex = xSemaphoreCreateMutex();

  configASSERT(sensorToControlQueue != nullptr);
  configASSERT(actuatorCmdQueue != nullptr);
  configASSERT(actuatorFeedbackQueue != nullptr);
  configASSERT(controlToCloudQueue != nullptr);
  configASSERT(cloudTelemetryQueue != nullptr);
  configASSERT(latestSensorMutex != nullptr);
  configASSERT(latestPumpStateMutex != nullptr);
}

void createTasks() {
  xTaskCreatePinnedToCore(Task_Actuator, "Task_Actuator", 4096, nullptr, PRIORITY_TASK_ACTUATOR, nullptr, 1);
  xTaskCreatePinnedToCore(Task_Control, "Task_Control", 6144, nullptr, PRIORITY_TASK_CONTROL, nullptr, 1);
  xTaskCreatePinnedToCore(Task_Sensor, "Task_Sensor", 4096, nullptr, PRIORITY_TASK_SENSOR, nullptr, 1);
  xTaskCreatePinnedToCore(Task_Feedback, "Task_Feedback", 4096, nullptr, PRIORITY_TASK_FEEDBACK, nullptr, 1);
  xTaskCreatePinnedToCore(Task_Cloud, "Task_Cloud", 8192, nullptr, PRIORITY_TASK_CLOUD, nullptr, 0);
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);

  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);

  sensingBegin();
  controlBegin();
  actuatorBegin();
  cloudBegin();

  createQueues();
  createTasks();

  Serial.println("ESP32 IoT irrigation system started");
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
