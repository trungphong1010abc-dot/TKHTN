#include "common_types_layer_flow.h"

#define PUMP_GPIO_PIN       26
#define ACTIVE_HIGH_RELAY   1

static void PumpGpio_Init(void)
{
    /*
     * Code thật ESP32: gpio_config(...).
     * Code thật STM32: MX_GPIO_Init() hoặc HAL_GPIO_Init(...).
     */
}

static void PumpGpio_Write(bool enable)
{
    /*
     * Code thật ESP32:
     * gpio_set_level(PUMP_GPIO_PIN, enable ? 1 : 0);
     *
     * Code thật STM32:
     * HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin,
     *                   enable ? GPIO_PIN_SET : GPIO_PIN_RESET);
     */
    (void)enable;
}

static bool IsPumpCommandValid(PumpCmd_t pump_cmd)
{
    return pump_cmd == PUMP_CMD_ON || pump_cmd == PUMP_CMD_OFF;
}

void Task_Actuator(void *argument)
{
    (void)argument;

    ActuatorCmd_t cmd = {0};
    ActuatorFeedback_t feedback = {0};

    PumpGpio_Init();
    PumpGpio_Write(false);

    for (;;) {
        if (xQueueReceive(actuatorCmdQueue, &cmd, portMAX_DELAY) != pdPASS) {
            continue;
        }

        feedback.timestamp = xTaskGetTickCount();
        feedback.watering_time_ms = 0;

        if (!IsPumpCommandValid(cmd.pump_cmd)) {
            PumpGpio_Write(false);
            feedback.pump_state = PUMP_STATE_OFF;
            feedback.relay_state = RELAY_STATE_OFF;
            feedback.actuator_status = ACTUATOR_STATUS_INVALID_CMD;
            xQueueSend(actuatorFeedbackQueue, &feedback, 0);
            continue;
        }

        if (cmd.pump_cmd == PUMP_CMD_ON) {
            PumpGpio_Write(true);
            feedback.pump_state = PUMP_STATE_ON;
            feedback.relay_state = RELAY_STATE_ON;
            feedback.actuator_status = ACTUATOR_STATUS_OK;

            vTaskDelay(pdMS_TO_TICKS(cmd.watering_duration_ms));

            PumpGpio_Write(false);
            feedback.pump_state = PUMP_STATE_OFF;
            feedback.relay_state = RELAY_STATE_OFF;
            feedback.watering_time_ms = cmd.watering_duration_ms;
            feedback.timestamp = xTaskGetTickCount();
        } else {
            PumpGpio_Write(false);
            feedback.pump_state = PUMP_STATE_OFF;
            feedback.relay_state = RELAY_STATE_OFF;
            feedback.actuator_status = ACTUATOR_STATUS_OK;
        }

        xQueueSend(actuatorFeedbackQueue, &feedback, 0);
    }
}
