#include "pump_controller.h"
#include "config.h"

static bool pumpRunning = false;
static unsigned long pumpStartedMs = 0;
static unsigned long pumpDurationMs = 0;
static unsigned long lastPumpStopMs = 0;
static bool hasPumpStoppedBefore = false;
static SystemMode currentMode = AUTO_MODE;

void pumpBegin() {
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL);
  pumpRunning = false;
}

void pumpUpdate() {
  if (!pumpRunning || pumpDurationMs == 0) {
    return;
  }

  if (millis() - pumpStartedMs >= pumpDurationMs) {
    stopPump();
  }
}

void startPump(unsigned long durationSec) {
  digitalWrite(RELAY_PIN, RELAY_ACTIVE_LEVEL);
  pumpRunning = true;
  pumpStartedMs = millis();
  pumpDurationMs = durationSec * 1000UL;
}

void stopPump() {
  digitalWrite(RELAY_PIN, RELAY_INACTIVE_LEVEL);

  if (pumpRunning) {
    lastPumpStopMs = millis();
    hasPumpStoppedBefore = true;
  }

  pumpRunning = false;
  pumpDurationMs = 0;
}

bool isPumpRunning() {
  return pumpRunning;
}

bool pumpCanStart(unsigned long minimumIntervalMs) {
  if (pumpRunning) {
    return false;
  }

  if (!hasPumpStoppedBefore) {
    return true;
  }

  return millis() - lastPumpStopMs >= minimumIntervalMs;
}

WateringDecision calculateWateringDecision(float soilPercent,
                                           float temperature,
                                           float humidity,
                                           int stage) {
  WateringDecision decision{};
  decision.pumpTimeSec = 0;
  decision.soilStatus = "Unknown";
  decision.floodWarning = false;
  decision.shouldWater = false;

  switch (stage) {
    case 1:
      if (soilPercent > 70.0f) {
        decision.soilStatus = "Too Wet";
        decision.floodWarning = true;
      } else if (soilPercent >= 55.0f) {
        decision.soilStatus = "Adequate";
      } else if (soilPercent > 40.0f) {
        decision.soilStatus = "Slightly Dry";
        decision.pumpTimeSec = 5;
      } else if (soilPercent > 25.0f) {
        decision.soilStatus = "Dry";
        decision.pumpTimeSec = 8;
      } else {
        decision.soilStatus = "Very Dry";
        decision.pumpTimeSec = 15;
      }

      if (decision.pumpTimeSec > 0 && temperature > 32.0f) {
        decision.pumpTimeSec += 3;
      }
      if (decision.pumpTimeSec > 0 && humidity < 50.0f && soilPercent <= 25.0f) {
        decision.pumpTimeSec += 5;
      }
      if (decision.pumpTimeSec > 0 && humidity > 80.0f) {
        decision.pumpTimeSec -= 2;
      }
      break;

    case 2:
      if (soilPercent > 75.0f) {
        decision.soilStatus = "Too Wet";
        decision.floodWarning = true;
      } else if (soilPercent >= 45.0f) {
        decision.soilStatus = "Adequate";
      } else if (soilPercent >= 30.0f) {
        decision.soilStatus = "Dry";
        decision.pumpTimeSec = 10;
      } else {
        decision.soilStatus = "Very Dry";
        decision.pumpTimeSec = 18;
      }

      if (decision.pumpTimeSec > 0 && temperature > 32.0f) {
        decision.pumpTimeSec += 5;
      }
      if (decision.pumpTimeSec > 0 && humidity < 45.0f) {
        decision.pumpTimeSec += 5;
      }
      if (decision.pumpTimeSec > 0 && humidity > 80.0f) {
        decision.pumpTimeSec -= 2;
      }
      break;

    case 3:
      if (soilPercent > 80.0f) {
        decision.soilStatus = "Too Wet";
        decision.floodWarning = true;
      } else if (soilPercent >= 40.0f) {
        decision.soilStatus = "Adequate";
      } else if (soilPercent >= 25.0f) {
        decision.soilStatus = "Dry";
        decision.pumpTimeSec = 10;
      } else {
        decision.soilStatus = "Very Dry";
        decision.pumpTimeSec = 20;
      }

      if (decision.pumpTimeSec > 0 && temperature > 35.0f) {
        decision.pumpTimeSec += 5;
      }
      if (decision.pumpTimeSec > 0 && humidity < 45.0f) {
        decision.pumpTimeSec += 3;
      }
      if (decision.pumpTimeSec > 0 && humidity > 80.0f) {
        decision.pumpTimeSec -= 2;
      }
      break;

    default:
      decision.soilStatus = "Unknown";
      break;
  }

  if (decision.pumpTimeSec <= 0) {
    decision.pumpTimeSec = 0;
    decision.shouldWater = false;
    return decision;
  }

  decision.pumpTimeSec = constrain(decision.pumpTimeSec,
                                   PUMP_TIME_MIN_SEC,
                                   PUMP_TIME_MAX_SEC);
  decision.shouldWater = true;
  return decision;
}

int calculatePumpTime(float soilPercent, float temperature, float humidity, int stage) {
  return calculateWateringDecision(soilPercent, temperature, humidity, stage).pumpTimeSec;
}

void setCurrentMode(SystemMode mode) {
  currentMode = mode;

  if (currentMode == AUTO_MODE && pumpDurationMs == 0) {
    stopPump();
  }
}

SystemMode getCurrentMode() {
  return currentMode;
}

const char* modeToString(SystemMode mode) {
  return mode == AUTO_MODE ? "AUTO" : "MANUAL";
}

bool parseMode(const String& value, SystemMode& mode) {
  if (value.equalsIgnoreCase("AUTO") || value.equalsIgnoreCase("A")) {
    mode = AUTO_MODE;
    return true;
  }

  if (value.equalsIgnoreCase("MANUAL") || value.equalsIgnoreCase("M")) {
    mode = MANUAL_MODE;
    return true;
  }

  return false;
}
