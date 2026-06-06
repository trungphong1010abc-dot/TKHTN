#include "common_types_layer_flow.h"

QueueHandle_t actuatorCmdQueue;
QueueHandle_t actuatorFeedbackQueue;
QueueHandle_t controlToCloudQueue;

void LayerFlow_CreateQueues(void)
{
    actuatorCmdQueue = xQueueCreate(4, sizeof(ActuatorCmd_t));
    actuatorFeedbackQueue = xQueueCreate(4, sizeof(ActuatorFeedback_t));
    controlToCloudQueue = xQueueCreate(4, sizeof(ControlData_t));
}
