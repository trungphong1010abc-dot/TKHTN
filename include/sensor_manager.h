#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

struct SensorData {
  float temperature;
  float humidity;
  int soilAdcRaw;
  int soilAdcFiltered;
  float soilPercent;
  bool dhtValid;
  bool soilValid;
  bool errorFlag;
  unsigned long timestamp;
};

void sensorBegin();
SensorData readSensors();
float calculateSoilPercent(int adcRaw);
const char* sensorStatusText(bool valid);

#endif
