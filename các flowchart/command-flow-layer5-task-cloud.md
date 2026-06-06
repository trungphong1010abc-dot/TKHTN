# Command flow - Layer 5 Task_Cloud

Nguồn chốt: `flow task cloud (1).jpg`, `flow các layer.jpg`, `design-rules-bien-va-chu-ky.md`

Biến tạo ra để gửi sang layer khác:

```text
TelemetryPacket_t
```

Biến cập nhật nội bộ trong layer:

```text
Không có
```

Biến nhận từ layer trước:

```text
SensorData_t
ControlData_t
```

Biến trong `SensorData_t`:

```text
T_air
H_air
H_soil
DHT_status
Soil_status
Error_Flag
timestamp
```

Biến trong `ControlData_t`:

```text
H_soil
H_threshold
pump_state
control_status
watering_duration
timestamp
```

Nguyên tắc của Layer 5:

```text
SensorData_t
ControlData_t
-> Task_Cloud
-> TelemetryPacket_t
-> Layer 6
```

Layer 5 là tầng đóng gói dữ liệu gửi cloud. Layer này nhận dữ liệu cảm biến và dữ liệu điều khiển, sau đó tạo `TelemetryPacket_t` để Layer 6 publish lên Cloud/Dashboard. Layer 5 không kiểm tra `wifi_status`, không kiểm tra `cloud_status` và không publish MQTT.

---

# Layer 5 - Task_Cloud

```text
[Layer 5. Task_Cloud]
Đối tượng: ESP32 + Task_Cloud
Công việc: Nhận SensorData_t và ControlData_t, đóng gói thành TelemetryPacket_t
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] SensorData_t, ControlData_t, TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: TelemetryPacket_t là gói dữ liệu tổng hợp để gửi lên Cloud/Dashboard. Nó được tạo ở Task_Cloud từ SensorData_t và ControlData_t.
```

v

```text
[L5-1. Nhận SensorData_t]
Đối tượng: Task_Cloud
Công việc: Nhận dữ liệu cảm biến mới nhất
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t cung cấp dữ liệu đo và trạng thái cảm biến để đưa vào telemetry.
```

v

```text
[L5-2. Nhận ControlData_t]
Đối tượng: Task_Cloud
Công việc: Nhận dữ liệu điều khiển từ Layer 2
[BIẾN VÀO] ControlData_t
[BIẾN DÙNG] H_soil, H_threshold, pump_state, control_status, watering_duration, timestamp
[BIẾN RA] ControlData_t
Giải thích: ControlData_t cung cấp kết quả điều khiển tưới, trạng thái bơm và thời lượng tưới để đưa vào telemetry.
```

v

```text
[L5-3. Tạo TelemetryPacket_t]
Đối tượng: Task_Cloud
Công việc: Gộp SensorData_t và ControlData_t thành gói telemetry
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: Đây là nơi TelemetryPacket_t được tạo ra. Layer 6 chỉ nhận gói này để publish, không tự tạo TelemetryPacket_t.
```

v

```text
[L5-4. Gửi TelemetryPacket_t sang Layer 6]
Đối tượng: Task_Cloud
Công việc: Chuyển TelemetryPacket_t cho tầng IoT Communication & Cloud Layer
[BIẾN VÀO] TelemetryPacket_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: Sau khi đóng gói xong, Task_Cloud chuyển TelemetryPacket_t sang Layer 6 để kiểm tra kết nối và publish lên Cloud/Dashboard.
```

---

# Mũi tên nối Layer 5

```text
Layer 1 hoặc Layer 4 [Đóng gói SensorData_t]
-> Layer 5 [L5-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```

```text
Layer 2 [Đóng gói ControlData_t]
-> Layer 5 [L5-2. Nhận ControlData_t]
Biến truyền: ControlData_t
```

```text
Layer 5 [L5-4. Gửi TelemetryPacket_t sang Layer 6]
-> Layer 6 [L6-1. Nhận TelemetryPacket_t]
Biến truyền: TelemetryPacket_t
```
