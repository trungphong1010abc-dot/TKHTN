#include "actuator.h"

#include "config.h"

void actuatorBegin() {
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_ACTIVE_HIGH ? LOW : HIGH);
}

PumpState_t applyPumpCmd(const PumpCmd_t &command) {
  const bool outputHigh = command.pump_cmd == PumpState::ON ? PUMP_ACTIVE_HIGH : !PUMP_ACTIVE_HIGH;
  digitalWrite(PUMP_PIN, outputHigh ? HIGH : LOW);

  PumpState_t state;
  state.pump_state = command.pump_cmd;
  state.timestamp = millis();
  return state;
}