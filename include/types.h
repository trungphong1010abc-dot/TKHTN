#pragma once

#include <Arduino.h>

enum class Status : uint8_t {
  OK,
  ERROR
};

enum class PumpState : uint8_t {
  OFF,
  ON
};

enum class ControlStatus : uint8_t {
  INIT,
  SOIL_ERROR,
  SOIL_MOISTURE_OK,
  TOO_WET,
  SAFETY_LOCK,
  WATERING,
  WATERING_DONE
};

struct DHTData_t {
  float T_air = NAN;
  float H_air = NAN;
  Status DHT_status = Status::ERROR;
  bool DHT_Error_Flag = true;
};

struct SoilData_t {
  uint16_t ADC_filtered = 0;
  float H_soil = NAN;
  Status Soil_status = Status::ERROR;
  bool Soil_Error_Flag = true;
};

struct SensorData_t {
  DHTData_t dhtData;
  SoilData_t soilData;
  bool Error_Flag = true;
  uint32_t timestamp = 0;
};

struct PumpCmd_t {
  PumpState pump_cmd = PumpState::OFF;
  uint32_t timestamp = 0;
};

struct PumpState_t {
  PumpState pump_state = PumpState::OFF;
  uint32_t timestamp = 0;
};

struct ControlData_t {
  bool data_valid = false;
  float T_max = NAN;
  float T_min = NAN;
  float T_avg = NAN;
  float GDD = 0.0f;
  float CGDD = 0.0f;
  uint8_t current_stage = 1;
  char soil_state[16] = "unknown";
  float H_threshold = 0.0f;
  uint32_t WATER_DURATION_MS = 0;
  uint32_t watering_duration = 0;
  PumpState pump_cmd = PumpState::OFF;
  ControlStatus control_status = ControlStatus::INIT;
  uint32_t timestamp = 0;
};

struct TelemetryPacket_t {
  SensorData_t SensorData;
  ControlData_t ControlData;
  PumpState_t PumpStateData;
  bool wifi_status = false;
  bool cloud_status = false;
  uint32_t timestamp = 0;
};

inline const char *statusToText(Status status) {
  return status == Status::OK ? "OK" : "ERROR";
}

inline const char *pumpStateToText(PumpState state) {
  return state == PumpState::ON ? "ON" : "OFF";
}

inline const char *controlStatusToText(ControlStatus status) {
  switch (status) {
    case ControlStatus::INIT:
      return "INIT";
    case ControlStatus::SOIL_ERROR:
      return "SOIL_ERROR";
    case ControlStatus::SOIL_MOISTURE_OK:
      return "SOIL_MOISTURE_OK";
    case ControlStatus::TOO_WET:
      return "TOO_WET";
    case ControlStatus::SAFETY_LOCK:
      return "SAFETY_LOCK";
    case ControlStatus::WATERING:
      return "WATERING";
    case ControlStatus::WATERING_DONE:
      return "WATERING_DONE";
    default:
      return "UNKNOWN";
  }
}
