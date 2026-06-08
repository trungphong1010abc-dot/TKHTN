#pragma once

#include "types.h"

void cloudBegin();
void cloudLoop();
TelemetryPacket_t makeTelemetryPacket(const SensorData_t &SensorData, const ControlData_t &ControlData, const PumpState_t &PumpStateData);
bool publishTelemetryPacket(const TelemetryPacket_t &TelemetryPacket);
