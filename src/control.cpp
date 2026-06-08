#include "control.h"

#include <ctime>
#include <cstring>

#include "config.h"

namespace {
uint32_t last_watering_time = 0;
float T_max = NAN;
float T_min = NAN;
float GDD = 0.0f;
float CGDD = 0.0f;
uint8_t current_stage = 1;
int active_gdd_day = -1;
int last_finalized_gdd_day = -1;

void setSoilState(ControlData_t &data, const char *soil_state, float H_threshold, uint32_t WATER_DURATION_MS) {
  strlcpy(data.soil_state, soil_state, sizeof(data.soil_state));
  data.H_threshold = H_threshold;
  data.WATER_DURATION_MS = WATER_DURATION_MS;
}

uint8_t stageFromCGDD(float value) {
  if (value >= STAGE_3_CGDD) {
    return 3;
  }
  if (value >= STAGE_2_CGDD) {
    return 2;
  }
  return 1;
}

int localDayKey(const tm &timeinfo) {
  return (timeinfo.tm_year + 1900) * 1000 + timeinfo.tm_yday;
}

void updateTemperatureExtremes(float T_air) {
  if (isnan(T_max) || isnan(T_min)) {
    T_max = T_air;
    T_min = T_air;
    return;
  }

  T_max = max(T_max, T_air);
  T_min = min(T_min, T_air);
}

void finalizeDailyGDD(int day_key) {
  if (day_key == last_finalized_gdd_day || isnan(T_max) || isnan(T_min)) {
    return;
  }

  const float T_avg = (T_max + T_min) / 2.0f;
  GDD = max(0.0f, T_avg - T_base);
  CGDD += GDD;
  current_stage = stageFromCGDD(CGDD);
  last_finalized_gdd_day = day_key;
}

void updateGDD(float T_air) {
  tm timeinfo;
  if (!getLocalTime(&timeinfo, 20)) {
    updateTemperatureExtremes(T_air);
    return;
  }

  const int current_day = localDayKey(timeinfo);
  if (active_gdd_day < 0) {
    active_gdd_day = current_day;
  }

  if (current_day != active_gdd_day) {
    finalizeDailyGDD(active_gdd_day);
    active_gdd_day = current_day;
    T_max = T_air;
    T_min = T_air;
    return;
  }

  updateTemperatureExtremes(T_air);

  if (timeinfo.tm_hour == 23 && timeinfo.tm_min >= 59) {
    finalizeDailyGDD(current_day);
  }
}

void chooseWaterRule(ControlData_t &data, float H_soil) {
  if (current_stage == 1) {
    if (H_soil > 70.0f) {
      setSoilState(data, "too_wet", 70.0f, 0);
    } else if (H_soil >= 55.0f) {
      setSoilState(data, "enough", 55.0f, 0);
    } else if (H_soil >= 40.0f) {
      setSoilState(data, "slightly_dry", 55.0f, 5000);
    } else if (H_soil >= 25.0f) {
      setSoilState(data, "dry", 40.0f, 8000);
    } else {
      setSoilState(data, "very_dry", 25.0f, 15000);
    }
    return;
  }

  if (current_stage == 2) {
    if (H_soil > 75.0f) {
      setSoilState(data, "too_wet", 75.0f, 0);
    } else if (H_soil >= 45.0f) {
      setSoilState(data, "enough", 45.0f, 0);
    } else if (H_soil >= 30.0f) {
      setSoilState(data, "dry", 45.0f, 10000);
    } else {
      setSoilState(data, "very_dry", 30.0f, 18000);
    }
    return;
  }

  if (H_soil > 80.0f) {
    setSoilState(data, "too_wet", 80.0f, 0);
  } else if (H_soil >= 40.0f) {
    setSoilState(data, "enough", 40.0f, 0);
  } else if (H_soil >= 25.0f) {
    setSoilState(data, "dry", 40.0f, 10000);
  } else {
    setSoilState(data, "very_dry", 25.0f, 20000);
  }
}

void adjustWaterDuration(ControlData_t &data, float T_air, float H_air, float H_soil) {
  if (data.WATER_DURATION_MS == 0 || isnan(T_air) || isnan(H_air)) {
    return;
  }

  int32_t duration = static_cast<int32_t>(data.WATER_DURATION_MS);
  if (current_stage == 1) {
    if (T_air > 32.0f) duration += 3000;
    if (H_air < 50.0f && H_soil <= 25.0f) duration += 5000;
    if (H_air > 90.0f) duration -= 2000;
  } else if (current_stage == 2) {
    if (T_air > 32.0f) duration += 5000;
    if (H_air < 45.0f) duration += 5000;
    if (H_air > 90.0f) duration -= 2000;
  } else {
    if (T_air > 35.0f) duration += 5000;
    if (H_air < 45.0f) duration += 3000;
    if (H_air > 90.0f) duration -= 2000;
  }

  data.WATER_DURATION_MS = static_cast<uint32_t>(max<int32_t>(duration, 0));
}
}  // namespace

void controlBegin() {
  last_watering_time = 0;
  active_gdd_day = -1;
  last_finalized_gdd_day = -1;
}

ControlData_t processControlData(const SensorData_t &SensorData) {
  ControlData_t data;
  data.timestamp = millis();
  data.T_max = T_max;
  data.T_min = T_min;
  data.GDD = GDD;
  data.CGDD = CGDD;
  data.current_stage = current_stage;

  const bool soilOk = SensorData.soilData.Soil_status == Status::OK;
  const bool dhtOk = SensorData.dhtData.DHT_status == Status::OK;
  data.data_valid = soilOk && dhtOk;

  if (!soilOk) {
    data.pump_cmd = PumpState::OFF;
    data.control_status = ControlStatus::SOIL_ERROR;
    return data;
  }

  if (dhtOk) {
    updateGDD(SensorData.dhtData.T_air);
    current_stage = stageFromCGDD(CGDD);
  }

  data.T_max = T_max;
  data.T_min = T_min;
  data.T_avg = !isnan(T_max) && !isnan(T_min) ? (T_max + T_min) / 2.0f : NAN;
  data.GDD = GDD;
  data.CGDD = CGDD;
  data.current_stage = current_stage;

  chooseWaterRule(data, SensorData.soilData.H_soil);
  adjustWaterDuration(data, SensorData.dhtData.T_air, SensorData.dhtData.H_air, SensorData.soilData.H_soil);

  if (data.WATER_DURATION_MS == 0) {
    data.pump_cmd = PumpState::OFF;
    data.control_status = strcmp(data.soil_state, "too_wet") == 0 ? ControlStatus::TOO_WET : ControlStatus::SOIL_MOISTURE_OK;
    return data;
  }

  const uint32_t now = millis();
  if (last_watering_time != 0 && now - last_watering_time < MIN_WATER_INTERVAL) {
    data.pump_cmd = PumpState::OFF;
    data.control_status = ControlStatus::SAFETY_LOCK;
    return data;
  }

  data.pump_cmd = PumpState::ON;
  data.watering_duration = data.WATER_DURATION_MS;
  data.control_status = ControlStatus::WATERING;
  last_watering_time = now;
  return data;
}
