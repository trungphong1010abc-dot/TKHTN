#include "gdd_manager.h"
#include "config.h"
#include <Arduino.h>

float calculateGDD(float tMax, float tMin) {
  if (isnan(tMax) || isnan(tMin)) {
    return 0.0f;
  }

  const float gdd = ((tMax + tMin) / 2.0f) - TBASE_C;
  return gdd < 0.0f ? 0.0f : gdd;
}

int determineStage(float cgdd) {
  if (cgdd < STAGE_1_MAX_CGDD) {
    return 1;
  }

  if (cgdd < STAGE_2_MAX_CGDD) {
    return 2;
  }

  return 3;
}

const char* stageName(int stage) {
  switch (stage) {
    case 1:
      return "Stage 1";
    case 2:
      return "Stage 2";
    case 3:
      return "Stage 3";
    default:
      return "Unknown";
  }
}
