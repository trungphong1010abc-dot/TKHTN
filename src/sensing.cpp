#include "sensing.h"

#include <DHT.h>

#include "config.h"

namespace {
DHT dht(DHT_PIN, DHT_TYPE);

uint16_t readADCFiltered() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < SOIL_ADC_SAMPLES; ++i) {
    sum += analogRead(SOIL_ADC_PIN);
    delay(5);
  }
  return static_cast<uint16_t>(sum / SOIL_ADC_SAMPLES);
}

float adcToHSoil(uint16_t ADC_filtered) {
  const int32_t range = static_cast<int32_t>(SOIL_ADC_DRY) - SOIL_ADC_WET;
  if (range == 0) {
    return NAN;
  }

  float H_soil = (static_cast<float>(SOIL_ADC_DRY) - ADC_filtered) * 100.0f / range;
  return constrain(H_soil, 0.0f, 100.0f);
}

SoilData_t readSoilData() {
  SoilData_t data;
  data.ADC_filtered = readADCFiltered();
  data.H_soil = adcToHSoil(data.ADC_filtered);

  const bool adcInRange = data.ADC_filtered > 0 && data.ADC_filtered < 4095;
  const bool moistureValid = !isnan(data.H_soil) && data.H_soil >= 0.0f && data.H_soil <= 100.0f;
  data.Soil_status = adcInRange && moistureValid ? Status::OK : Status::ERROR;
  data.Soil_Error_Flag = data.Soil_status != Status::OK;
  return data;
}

DHTData_t readDHTData() {
  DHTData_t data;
  data.T_air = dht.readTemperature();
  data.H_air = dht.readHumidity();

  const bool valid = !isnan(data.T_air) && !isnan(data.H_air) &&
                     data.T_air > -20.0f && data.T_air < 80.0f &&
                     data.H_air >= 0.0f && data.H_air <= 100.0f;
  data.DHT_status = valid ? Status::OK : Status::ERROR;
  data.DHT_Error_Flag = data.DHT_status != Status::OK;
  return data;
}
}  // namespace

void sensingBegin() {
  pinMode(SOIL_ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(SOIL_ADC_PIN, ADC_11db);
  dht.begin();
  delay(DHT_STABILIZE_DELAY_MS);
}

SensorData_t readSensorData() {
  SensorData_t data;
  data.dhtData = readDHTData();
  data.soilData = readSoilData();
  data.Error_Flag = data.dhtData.DHT_Error_Flag || data.soilData.Soil_Error_Flag;
  data.timestamp = millis();
  return data;
}

SensorData_t readFeedbackSensorData(const SensorData_t &latestSensorData) {
  SensorData_t data = latestSensorData;
  data.soilData = readSoilData();
  data.Error_Flag = data.dhtData.DHT_Error_Flag || data.soilData.Soil_Error_Flag;
  data.timestamp = millis();
  return data;
}
