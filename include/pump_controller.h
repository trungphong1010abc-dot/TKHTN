#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>

enum SystemMode {
  AUTO_MODE,
  MANUAL_MODE
};

struct WateringDecision {
  int pumpTimeSec;
  const char* soilStatus;
  bool floodWarning;
  bool shouldWater;
};

void pumpBegin();
void pumpUpdate();
void startPump(unsigned long durationSec = 0);
void stopPump();
bool isPumpRunning();
bool pumpCanStart(unsigned long minimumIntervalMs);

WateringDecision calculateWateringDecision(float soilPercent,
                                           float temperature,
                                           float humidity,
                                           int stage);
int calculatePumpTime(float soilPercent, float temperature, float humidity, int stage);

void setCurrentMode(SystemMode mode);
SystemMode getCurrentMode();
const char* modeToString(SystemMode mode);
bool parseMode(const String& value, SystemMode& mode);

#endif
