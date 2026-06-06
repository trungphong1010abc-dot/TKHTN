#ifndef THINGSBOARD_MANAGER_H
#define THINGSBOARD_MANAGER_H

#include <Arduino.h>
#include "pump_controller.h"

using ModeCommandCallback = void (*)(SystemMode mode);
using PumpCommandCallback = void (*)(bool turnOn);

void thingsBoardBegin();
void thingsBoardSetCommandCallbacks(ModeCommandCallback modeCallback,
                                    PumpCommandCallback pumpCallback);
void thingsBoardSetCurrentMode(SystemMode mode);
void thingsBoardLoop();
bool connectWiFi();
bool thingsBoardConnected();

bool publishTelemetry(float temperature,
                      float humidity,
                      float soilMoisture,
                      float tmax,
                      float tmin,
                      float gdd,
                      float cgdd,
                      int stage,
                      bool pumpState,
                      SystemMode mode,
                      const WateringDecision& wateringDecision);

#endif
