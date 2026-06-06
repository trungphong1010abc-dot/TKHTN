#ifndef COMMON_TYPES_LAYER_FLOW_H
#define COMMON_TYPES_LAYER_FLOW_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"

typedef enum {
    PUMP_CMD_OFF = 0,
    PUMP_CMD_ON  = 1
} PumpCmd_t;

typedef enum {
    PUMP_STATE_OFF = 0,
    PUMP_STATE_ON  = 1
} PumpState_t;

typedef enum {
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON  = 1
} RelayState_t;

typedef enum {
    ACTUATOR_STATUS_OK = 0,
    ACTUATOR_STATUS_BLOCKED_BY_ERROR,
    ACTUATOR_STATUS_INVALID_CMD
} ActuatorStatus_t;

typedef struct {
    float T_air;
    float H_air;
    float H_soil;
    bool DHT_status;
    bool Soil_status;
    bool Error_Flag;
    uint32_t timestamp;
} SensorData_t;

typedef struct {
    PumpCmd_t pump_cmd;
    uint32_t watering_duration_ms;
    uint32_t timestamp;
} ActuatorCmd_t;

typedef struct {
    PumpState_t pump_state;
    RelayState_t relay_state;
    ActuatorStatus_t actuator_status;
    uint32_t watering_time_ms;
    uint32_t timestamp;
} ActuatorFeedback_t;

typedef struct {
    PumpCmd_t pump_cmd;
    PumpState_t pump_state;
    RelayState_t relay_state;
    ActuatorStatus_t actuator_status;
    uint32_t watering_duration_ms;
    uint32_t watering_time_ms;
    uint32_t timestamp;
} ControlData_t;

typedef struct {
    char device_id[24];
    SensorData_t sensor;
    ControlData_t control;
    uint32_t timestamp;
} TelemetryPacket_t;

extern QueueHandle_t actuatorCmdQueue;
extern QueueHandle_t actuatorFeedbackQueue;
extern QueueHandle_t controlToCloudQueue;

#endif
