#ifndef GDD_MANAGER_H
#define GDD_MANAGER_H

float calculateGDD(float tMax, float tMin);
int determineStage(float cgdd);
const char* stageName(int stage);

#endif
