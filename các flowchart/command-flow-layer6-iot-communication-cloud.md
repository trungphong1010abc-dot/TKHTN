# Command flow - Layer 6 IoT Communication & Cloud

Nguồn chốt: `các tầng khác chưa chọn flow.jpg`, `flow task cloud (1).jpg`, `flow các layer.jpg`, `design-rules-bien-va-chu-ky.md`, `review-tong-quan-5-tang-command-flow.md`, code mẫu `03_task_cloud_dong_goi_log.c`

Layer 6 trong file này là tầng truyền thông IoT và cloud. Tầng này nhận dữ liệu đã được Task_Cloud chuẩn bị, kiểm tra trạng thái kết nối và publish dữ liệu lên Cloud/Dashboard.

```text
Layer 5 [Task_Cloud / đóng gói TelemetryPacket_t]
-> Layer 6 [IoT Communication & Cloud Layer]
-> Cloud/Dashboard
```

## Biến Layer 6 dùng

```text
DHTData
SoilData
SensorData_t
ControlData_t
TelemetryPacket_t
wifi_status
cloud_status
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

Ghi chú:

```text
Ảnh flow dùng watering_duration trong ControlData_t.
Một số file code mẫu có thể dùng tên gần tương đương như watering_time hoặc watering_duration_ms.
Trong command-flow này giữ tên watering_duration theo ảnh đã chốt.
```

Không tự thêm:

```text
remote_config
ConfigData_t
dashboard_data
upload_status
cloud_ready
retry_count
manual_cmd
auto_mode
```

---

## Layer 6 - IoT Communication & Cloud Layer

```text
[Layer 6. IoT Communication & Cloud Layer]
Đối tượng: Task_Cloud + MQTT Client/publisher
Công việc: Nhận dữ liệu cảm biến và điều khiển, kiểm tra kết nối, publish telemetry lên Cloud/Dashboard
[BIẾN VÀO] SensorData_t, ControlData_t, TelemetryPacket_t
[BIẾN DÙNG] wifi_status, cloud_status
[BIẾN RA] TelemetryPacket_t
Giải thích: Layer 6 là tầng truyền thông. Tầng này không quyết định tưới, không điều khiển relay và không tính GDD/CGDD. Nó chỉ lấy dữ liệu đã có, kiểm tra trạng thái WiFi/cloud và gửi dữ liệu lên Cloud/Dashboard.
```

v

```text
[L6-1. Nhận dữ liệu từ Task_Cloud]
Đối tượng: Task_Cloud
Công việc: Nhận dữ liệu đã được gom từ các layer trước
[BIẾN VÀO] DHTData, SoilData, SensorData_t, ControlData_t
[BIẾN DÙNG] SensorData_t, ControlData_t
[BIẾN RA] SensorData_t, ControlData_t
Giải thích: Task_Cloud nhận dữ liệu cảm biến và dữ liệu điều khiển. SensorData_t chứa trạng thái môi trường; ControlData_t chứa kết quả điều khiển tưới.
```

v

```text
[L6-2. Kiểm tra phần SensorData_t]
Đối tượng: SensorData_t
Công việc: Xác định các biến cảm biến sẽ được đưa vào gói telemetry
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t gồm nhiệt độ không khí, độ ẩm không khí, độ ẩm đất, trạng thái DHT22, trạng thái Soil Sensor, cờ lỗi chung và thời điểm đo.
```

v

```text
[L6-3. Kiểm tra phần ControlData_t]
Đối tượng: ControlData_t
Công việc: Xác định các biến điều khiển sẽ được đưa vào gói telemetry
[BIẾN VÀO] ControlData_t
[BIẾN DÙNG] H_soil, H_threshold, pump_state, control_status, watering_duration, timestamp
[BIẾN RA] ControlData_t
Giải thích: ControlData_t mô tả kết quả quyết định tưới và trạng thái bơm. H_soil và H_threshold cho biết cơ sở độ ẩm; pump_state cho biết trạng thái bơm; control_status cho biết lý do điều khiển; watering_duration cho biết thời gian tưới.
```

v

```text
[L6-4. Tạo hoặc cập nhật TelemetryPacket_t]
Đối tượng: TelemetryPacket_t
Công việc: Gom SensorData_t và ControlData_t thành gói telemetry thống nhất
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: TelemetryPacket_t là gói dữ liệu gửi lên Cloud/Dashboard. Gói này giúp cloud nhận cùng lúc dữ liệu cảm biến và dữ liệu điều khiển của một chu kỳ.
```

v

```text
[L6-5. Kiểm tra wifi_status]
Đối tượng: ESP32 WiFi
Công việc: Kiểm tra trạng thái kết nối WiFi trước khi publish
[BIẾN VÀO] wifi_status
[BIẾN DÙNG] wifi_status
[BIẾN RA] wifi_status
Giải thích: Nếu wifi_status không sẵn sàng thì Layer 6 chưa thể publish telemetry. File command-flow này không tự thêm cơ chế retry hoặc buffer nếu chưa được chốt trong design-rules.
```

Nếu `wifi_status` chưa sẵn sàng:

```text
[L6-E1. WiFi chưa sẵn sàng]
Đối tượng: ESP32 WiFi
Công việc: Không publish dữ liệu trong chu kỳ hiện tại
[BIẾN VÀO] wifi_status, TelemetryPacket_t
[BIẾN DÙNG] wifi_status
[BIẾN RA] cloud_status
Giải thích: Khi WiFi chưa sẵn sàng, telemetry không được gửi lên cloud. Layer 6 chỉ cập nhật trạng thái cloud_status theo tình trạng truyền thông, không thay đổi quyết định tưới.
```

Nếu `wifi_status` sẵn sàng:

v

```text
[L6-6. Kiểm tra cloud_status]
Đối tượng: MQTT Client / Cloud connection
Công việc: Kiểm tra trạng thái kết nối đến Cloud/Dashboard
[BIẾN VÀO] cloud_status
[BIẾN DÙNG] cloud_status
[BIẾN RA] cloud_status
Giải thích: cloud_status cho biết MQTT Client hoặc kênh truyền cloud đã sẵn sàng publish hay chưa.
```

Nếu `cloud_status` chưa sẵn sàng:

```text
[L6-E2. Cloud chưa sẵn sàng]
Đối tượng: MQTT Client / Cloud connection
Công việc: Không publish dữ liệu trong chu kỳ hiện tại
[BIẾN VÀO] cloud_status, TelemetryPacket_t
[BIẾN DÙNG] cloud_status
[BIẾN RA] cloud_status
Giải thích: Khi cloud_status chưa sẵn sàng, telemetry chưa được gửi. Layer 6 không tự tạo biến upload_status hoặc retry_count vì các biến này chưa được chốt.
```

Nếu `cloud_status` sẵn sàng:

v

```text
[L6-7. Publish TelemetryPacket_t]
Đối tượng: MQTT Client/publisher
Công việc: Publish TelemetryPacket_t lên Cloud/Dashboard
[BIẾN VÀO] TelemetryPacket_t, wifi_status, cloud_status
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: Khi wifi_status và cloud_status đều sẵn sàng, MQTT Client/publisher gửi TelemetryPacket_t lên Cloud/Dashboard. Đây là điểm cuối của flow truyền dữ liệu lên cloud.
```

v

```text
[L6-8. Cloud/Dashboard nhận telemetry]
Đối tượng: Cloud/Dashboard
Công việc: Nhận dữ liệu cảm biến và điều khiển để hiển thị hoặc lưu trữ
[BIẾN VÀO] TelemetryPacket_t
[BIẾN DÙNG] SensorData_t, ControlData_t
[BIẾN RA] TelemetryPacket_t
Giải thích: Cloud/Dashboard nhận TelemetryPacket_t để biết trạng thái môi trường, trạng thái bơm và quyết định tưới của hệ thống.
```

---

## Mũi tên nối Layer 6

```text
Layer 1 / Signal Processing Layer [Đóng gói SensorData_t]
-> Layer 6 [L6-2. Kiểm tra phần SensorData_t]
Biến truyền: SensorData_t
Ghi chú: SensorData_t chứa T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp.
```

```text
Control Decision Layer [Đóng gói ControlData_t]
-> Layer 6 [L6-3. Kiểm tra phần ControlData_t]
Biến truyền: ControlData_t
Ghi chú: ControlData_t chứa H_soil, H_threshold, pump_state, control_status, watering_duration, timestamp.
```

```text
Layer 5 [Task_Cloud tạo TelemetryPacket_t]
-> Layer 6 [L6-4. Tạo hoặc cập nhật TelemetryPacket_t]
Biến truyền: TelemetryPacket_t
Ghi chú: Nếu tách Layer 5 và Layer 6 rõ ràng, Layer 5 chuẩn bị gói; Layer 6 chịu trách nhiệm truyền gói lên cloud.
```

```text
Layer 6 [L6-4. Tạo hoặc cập nhật TelemetryPacket_t]
-> Layer 6 [L6-5. Kiểm tra wifi_status]
Biến truyền: TelemetryPacket_t, wifi_status
```

```text
Layer 6 [L6-5. Kiểm tra wifi_status]
-> Layer 6 [L6-6. Kiểm tra cloud_status]
Biến truyền: TelemetryPacket_t, cloud_status
```

```text
Layer 6 [L6-6. Kiểm tra cloud_status]
-> Layer 6 [L6-7. Publish TelemetryPacket_t]
Biến truyền: TelemetryPacket_t
```

```text
Layer 6 [L6-7. Publish TelemetryPacket_t]
-> Cloud/Dashboard [L6-8. Cloud/Dashboard nhận telemetry]
Biến truyền: TelemetryPacket_t
Ghi chú: Đây là mũi tên cuối của flow IoT Communication & Cloud Layer.
```
