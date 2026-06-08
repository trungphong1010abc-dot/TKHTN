# Command flow - Task_Cloud

Vị trí:

```text
Task_Cloud không phải layer riêng.
Task_Cloud là task FreeRTOS thuộc Layer 5 - IoT Communication.
```

Luồng tổng quát:

```text
Layer 1 hoặc Layer 4
-> RTOS [cloudTelemetryQueue hoặc shared latest buffer]
-> Task_Cloud [nhận SensorData_t mới nhất]

Layer 2
-> RTOS [controlToCloudQueue]
-> Task_Cloud [nhận ControlData_t]

Task_Cloud
-> tạo TelemetryPacket_t
-> MQTT/JSON/TCP/IP/WiFi
-> ThingsBoard/Server
```

Biến:

```text
Đầu vào: SensorData_t, ControlData_t
Đầu ra: TelemetryPacket_t
Biến nội bộ Layer 5: wifi_status, cloud_status
```

Ghi chú:

```text
wifi_status lấy từ WiFi/Ethernet driver hoặc network stack.
cloud_status lấy từ kết quả MQTT connect/publish.
Hai biến này không đến từ Layer 1-4 và không quyết định pump_cmd.
```

---

# Flow chi tiết

```text
[TC-1. Nhận SensorData_t mới nhất]
Đối tượng: Task_Cloud
Công việc: Lấy dữ liệu cảm biến mới nhất theo timestamp
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t có thể đến từ Layer 1 hoặc Layer 4. Task_Cloud chọn bản mới nhất theo timestamp.
```

v

```text
[TC-2. Nhận ControlData_t]
Đối tượng: Task_Cloud
Công việc: Nhận dữ liệu điều khiển từ Task_Control
[BIẾN VÀO] ControlData_t
[BIẾN DÙNG] H_soil, H_threshold, current_stage, soil_state, pump_cmd, control_status, watering_duration, timestamp
[BIẾN RA] ControlData_t
Giải thích: ControlData_t mô tả quyết định tưới và trạng thái điều khiển tại ESP32.
```

v

```text
[TC-3. Tạo TelemetryPacket_t]
Đối tượng: Task_Cloud
Công việc: Ghép SensorData_t và ControlData_t thành telemetry
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: TelemetryPacket_t được tạo ở Task_Cloud thuộc Layer 5.
```

v

```text
[TC-4. Chuyển sang MQTT/JSON]
Đối tượng: Task_Cloud + MQTT Client/publisher
Công việc: Chuyển TelemetryPacket_t thành payload JSON để publish MQTT
[BIẾN VÀO] TelemetryPacket_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] MQTT message
Giải thích: Đây là bước ứng dụng của Layer 5, không phải queue nội bộ RTOS.
```

v

```text
[TC-5. Kiểm tra wifi_status và publish]
Đối tượng: Task_Cloud + WiFi/Ethernet + MQTT Client
Công việc: Kiểm tra mạng, publish telemetry và cập nhật cloud_status
[BIẾN VÀO] MQTT message
[BIẾN DÙNG] wifi_status, cloud_status, TelemetryPacket_t
[BIẾN RA] cloud_status
Giải thích: Mất WiFi/server chỉ làm telemetry chậm, không làm sai điều khiển tưới cục bộ.
```

v

```text
[TC-6. Quay lại chờ dữ liệu hoặc chu kỳ telemetry]
Đối tượng: Task_Cloud + FreeRTOS Scheduler
Công việc: Chờ dữ liệu mới hoặc chu kỳ publish tiếp theo
[BIẾN VÀO] Không có
[BIẾN DÙNG] vTaskDelay hoặc xQueueReceive với timeout
[BIẾN RA] Không có
Giải thích: Task_Cloud priority thấp hơn Task_Control và Task_Actuator để không làm chậm điều khiển.
```

---

# Không được hiểu sai

```text
Task_Cloud không quyết định tưới.
Task_Cloud không tạo pump_cmd.
Task_Cloud không điều khiển GPIO.
Task_Cloud không thay Layer 5.
Task_Cloud là một task bên trong Layer 5.
```

