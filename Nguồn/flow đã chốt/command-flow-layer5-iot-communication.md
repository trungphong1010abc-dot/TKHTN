# Command flow - Layer 5 IoT Communication

Vai trò:

```text
Layer 5 là tầng truyền thông IoT của hệ mạng cảm biến không dây.
Task_Cloud là task thuộc Layer 5, không phải layer riêng.
Layer 5 nhận SensorData_t và ControlData_t, tạo TelemetryPacket_t, gửi MQTT/JSON qua TCP/IP/WiFi tới ThingsBoard/Server.
```

Đối tượng:

```text
Task_Cloud
MQTT Client/publisher
TCP
IP
WiFi/Ethernet
ThingsBoard/Server
```

Biến đầu vào:

```text
SensorData_t
ControlData_t
```

Biến tạo ra:

```text
TelemetryPacket_t
```

Biến nội bộ Layer 5:

```text
wifi_status
cloud_status
```

Nguồn của biến nội bộ:

```text
wifi_status lấy từ WiFi/Ethernet driver hoặc network stack của ESP32.
cloud_status lấy từ trạng thái MQTT connect/publish tới ThingsBoard/Server.
Hai biến này không đến từ Layer 1, Layer 2, Layer 3 hoặc Layer 4.
Hai biến này không quyết định pump_cmd.
```

---

# Flow chi tiết

```text
[Layer 5. IoT Communication Layer]
Đối tượng: Task_Cloud + MQTT Client + TCP/IP + WiFi/Ethernet + ThingsBoard/Server
Công việc: Gửi telemetry từ ESP32 lên server
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] SensorData_t, ControlData_t, TelemetryPacket_t, wifi_status, cloud_status
[BIẾN RA] TelemetryPacket_t
Giải thích: Layer 5 chỉ làm truyền thông. Nếu WiFi/server lỗi, ESP32 vẫn điều khiển tưới cục bộ ở Layer 2/3/4.
```

v

```text
[L5-1. Task_Cloud nhận SensorData_t]
Đối tượng: Task_Cloud
Công việc: Lấy SensorData_t mới nhất theo timestamp
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t có thể đến từ Layer 1 hoặc Layer 4. Task_Cloud không chọn theo tên layer, chỉ chọn bản mới nhất theo timestamp.
```

v

```text
[L5-2. Task_Cloud nhận ControlData_t]
Đối tượng: Task_Cloud
Công việc: Nhận trạng thái điều khiển từ Layer 2
[BIẾN VÀO] ControlData_t
[BIẾN DÙNG] H_soil, H_threshold, current_stage, soil_state, pump_cmd, control_status, watering_duration, timestamp
[BIẾN RA] ControlData_t
Giải thích: ControlData_t cho server biết quyết định tưới và trạng thái điều khiển tại ESP32.
```

v

```text
[L5-3. Tạo TelemetryPacket_t]
Đối tượng: Task_Cloud
Công việc: Ghép SensorData_t và ControlData_t thành gói telemetry
[BIẾN VÀO] SensorData_t, ControlData_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TelemetryPacket_t
Giải thích: TelemetryPacket_t được tạo tại Layer 5, không phải truyền từ server về.
```

v

```text
[L5-4. Chuyển TelemetryPacket_t thành MQTT/JSON]
Đối tượng: MQTT Client/publisher
Công việc: Tạo payload JSON và chuẩn bị publish MQTT
[BIẾN VÀO] TelemetryPacket_t
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] MQTT message
Giải thích: MQTT/JSON là định dạng truyền thông lên ThingsBoard/Server.
```

v

```text
[L5-5. Kiểm tra wifi_status]
Đối tượng: WiFi/Ethernet driver hoặc network stack
Công việc: Kiểm tra ESP32 có kết nối mạng không
[BIẾN VÀO] Không có từ Layer 1-4
[BIẾN DÙNG] wifi_status
[BIẾN RA] wifi_status
Giải thích: wifi_status sinh ra trong Layer 5 từ trạng thái WiFi/Ethernet.
```

Nếu `wifi_status` không sẵn sàng:

```text
[L5-E1. Bỏ qua publish trong chu kỳ hiện tại]
Đối tượng: Task_Cloud
Công việc: Không gửi telemetry trong chu kỳ này
[BIẾN VÀO] wifi_status, TelemetryPacket_t
[BIẾN DÙNG] wifi_status, TelemetryPacket_t
[BIẾN RA] cloud_status = không xác nhận publish
Giải thích: Mất WiFi không được làm sai điều khiển tưới cục bộ.
```

Nếu `wifi_status` sẵn sàng:

v

```text
[L5-6. Truyền qua TCP]
Đối tượng: TCP
Công việc: Gửi MQTT message qua giao vận TCP
[BIẾN VÀO] MQTT message
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] TCP segment
Giải thích: MQTT chạy trên TCP.
```

v

```text
[L5-7. Truyền qua IP]
Đối tượng: IP
Công việc: Định tuyến dữ liệu tới địa chỉ ThingsBoard/Server
[BIẾN VÀO] TCP segment
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] IP packet
Giải thích: IP đưa dữ liệu từ ESP32 tới server qua mạng.
```

v

```text
[L5-8. Truyền qua WiFi/Ethernet]
Đối tượng: WiFi/Ethernet
Công việc: Đưa frame ra môi trường truyền
[BIẾN VÀO] IP packet
[BIẾN DÙNG] wifi_status
[BIẾN RA] frame mạng
Giải thích: Đây là phần truyền thông vật lý/liên kết của ESP32.
```

v

```text
[L5-9. Cập nhật cloud_status]
Đối tượng: MQTT Client/publisher
Công việc: Kiểm tra kết quả connect/publish
[BIẾN VÀO] kết quả MQTT publish
[BIẾN DÙNG] cloud_status
[BIẾN RA] cloud_status
Giải thích: cloud_status sinh ra từ kết quả MQTT, không phải biến server truyền ngược về Layer 2.
```

Nếu `cloud_status` không sẵn sàng:

```text
[L5-E2. Server chưa xác nhận telemetry]
Đối tượng: Task_Cloud
Công việc: Ghi nhận publish chưa thành công trong chu kỳ hiện tại
[BIẾN VÀO] cloud_status, TelemetryPacket_t
[BIẾN DÙNG] cloud_status, TelemetryPacket_t
[BIẾN RA] cloud_status
Giải thích: Cloud lỗi chỉ ảnh hưởng telemetry, không dừng điều khiển tưới cục bộ.
```

Nếu `cloud_status` sẵn sàng:

v

```text
[L5-10. ThingsBoard/Server nhận telemetry]
Đối tượng: ThingsBoard/Server
Công việc: Nhận MQTT message, đọc JSON, lưu và hiển thị telemetry
[BIẾN VÀO] TelemetryPacket_t qua MQTT
[BIẾN DÙNG] TelemetryPacket_t
[BIẾN RA] dữ liệu hiển thị trên server
Giải thích: ThingsBoard/Server là endpoint ngoài ESP32, không phải task nội bộ và không phải layer mới.
```

---

# Mũi tên nối layer

```text
Layer 1 hoặc Layer 4
-> RTOS [cloudTelemetryQueue hoặc shared latest buffer]
-> Layer 5 [L5-1. Task_Cloud nhận SensorData_t]
Biến truyền: SensorData_t
Quy tắc chọn: bản mới nhất theo timestamp.
```

```text
Layer 2 [L2-17. Gửi ControlData_t sang Layer 5]
-> RTOS [controlToCloudQueue]
-> Layer 5 [L5-2. Task_Cloud nhận ControlData_t]
Biến truyền: ControlData_t
```

```text
Layer 5 [L5-3. Tạo TelemetryPacket_t]
-> Layer 5 [L5-4 đến L5-10. MQTT/JSON/TCP/IP/WiFi/Server]
Biến truyền: TelemetryPacket_t
Ghi chú: đây là luồng truyền thông IoT, không phải queue nội bộ RTOS.
```

