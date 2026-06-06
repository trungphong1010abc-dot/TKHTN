#include "common_types_layer_flow.h"

#define H_SOIL_THRESHOLD_PERCENT 35.0f
#define WATERING_DURATION_MS     5000U

static PumpCmd_t DecidePumpCommand(const SensorData_t *sensor)
{
    if (sensor->Error_Flag || !sensor->DHT_status || !sensor->Soil_status) {
        return PUMP_CMD_OFF;
    }

    if (sensor->H_soil < H_SOIL_THRESHOLD_PERCENT) {
        return PUMP_CMD_ON;
    }

    return PUMP_CMD_OFF;
}

void Task_Control(void *argument)
{
    SensorData_t sensorData;
    ActuatorCmd_t actuatorCmd;
    ActuatorFeedback_t feedback;
    ControlData_t controlData;

    for (;;) {
        /*
         * Code thật: nhận sensorData từ sensorToControlQueue.
         * Ở file mẫu này, sensorData được giả định đã có dữ liệu mới.
         */

        actuatorCmd.pump_cmd = DecidePumpCommand(&sensorData);
        actuatorCmd.watering_duration_ms = WATERING_DURATION_MS;
        actuatorCmd.timestamp = xTaskGetTickCount();

        xQueueSend(actuatorCmdQueue, &actuatorCmd, 0);

        if (xQueueReceive(actuatorFeedbackQueue, &feedback, pdMS_TO_TICKS(1000)) == pdPASS) {
            controlData.pump_cmd = actuatorCmd.pump_cmd;
            controlData.pump_state = feedback.pump_state;
            controlData.relay_state = feedback.relay_state;
            controlData.actuator_status = feedback.actuator_status;
            controlData.watering_duration_ms = actuatorCmd.watering_duration_ms;
            controlData.watering_time_ms = feedback.watering_time_ms;
            controlData.timestamp = feedback.timestamp;

            xQueueSend(controlToCloudQueue, &controlData, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
