#include <string.h>
#include "common_types_layer_flow.h"

static void Cloud_SendTelemetry(const TelemetryPacket_t *packet)
{
    /*
     * Code thật:
     * - MQTT publish packet dưới dạng JSON.
     * - Hoặc HTTP POST packet lên server.
     */
    (void)packet;
}

void Task_Cloud(void *argument)
{
    ControlData_t controlData;
    SensorData_t latestSensorData;
    TelemetryPacket_t packet;

    memset(&packet, 0, sizeof(packet));
    strcpy(packet.device_id, "ESP32_WATERING_01");

    for (;;) {
        /*
         * Code thật: latestSensorData nên được cập nhật từ sensor queue
         * hoặc biến global được bảo vệ bằng mutex.
         */

        if (xQueueReceive(controlToCloudQueue, &controlData, portMAX_DELAY) == pdPASS) {
            packet.sensor = latestSensorData;
            packet.control = controlData;
            packet.timestamp = xTaskGetTickCount();

            Cloud_SendTelemetry(&packet);
        }
    }
}
