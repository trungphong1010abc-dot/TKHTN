#pragma once

#include "types.h"

void sensingBegin();
SensorData_t readSensorData();
SensorData_t readFeedbackSensorData(const SensorData_t &latestSensorData);

