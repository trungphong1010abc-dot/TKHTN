# Command flow - Layer 1 Sensing

Vai trò:

```text
Layer 1 chỉ đọc cảm biến, kiểm tra lỗi và đóng gói SensorData_t.
Layer 1 không quyết định tưới, không tạo pump_cmd, không điều khiển bơm, không publish MQTT.
```

Đối tượng:

```text
Task_Sensor
DHT22
Soil sensor điện dung
ADC ESP32
ESP32
```

Biến chính:

```text
T_air
H_air
H_soil
DHT_status
Soil_status
DHT_Error_Flag
Soil_Error_Flag
Error_Flag
ADC_filtered
timestamp
DHTData
SoilData
SensorData_t
```

---

# Flow chi tiết

```text
[Layer 1. Sensing Layer]
Đối tượng: Task_Sensor + DHT22 + Soil sensor + ADC ESP32
Công việc: Đọc nhiệt độ, độ ẩm không khí và độ ẩm đất, sau đó đóng gói SensorData_t
[BIẾN VÀO] Không có
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: Đây là tầng tạo dữ liệu đầu vào cho thuật toán điều khiển và telemetry.
```

v

```text
[L1-1. Khởi tạo và kiểm tra DHT22]
Đối tượng: Task_Sensor + DHT22
Công việc: Chuẩn bị đọc nhiệt độ và độ ẩm không khí
[BIẾN VÀO] Không có
[BIẾN DÙNG] DHT_status
[BIẾN RA] DHT_status
Giải thích: DHT_status cho biết DHT22 sẵn sàng hay lỗi trước khi đọc T_air và H_air.
```

v

```text
[L1-2. Đọc T_air và H_air]
Đối tượng: DHT22
Công việc: Đọc nhiệt độ không khí và độ ẩm không khí
[BIẾN VÀO] Không có
[BIẾN DÙNG] T_air, H_air
[BIẾN RA] T_air, H_air
Giải thích: T_air dùng cho GDD/CGDD và hiệu chỉnh tưới; H_air dùng để hiệu chỉnh thời lượng tưới.
```

v

```text
[L1-3. Kiểm tra lỗi DHT22]
Đối tượng: Task_Sensor
Công việc: Kiểm tra dữ liệu DHT22 có hợp lệ không
[BIẾN VÀO] T_air, H_air
[BIẾN DÙNG] DHT_status, DHT_Error_Flag, Error_Flag
[BIẾN RA] DHT_status, DHT_Error_Flag, Error_Flag
Giải thích: Nếu DHT22 lỗi, Layer 2 không cập nhật GDD/CGDD bằng dữ liệu lỗi.
```

v

```text
[L1-4. Đóng gói DHTData]
Đối tượng: Task_Sensor
Công việc: Gói dữ liệu nhánh DHT22
[BIẾN VÀO] T_air, H_air, DHT_status, DHT_Error_Flag, Error_Flag
[BIẾN DÙNG] DHTData
[BIẾN RA] DHTData
Giải thích: DHTData là dữ liệu trung gian trước khi ghép vào SensorData_t.
```

v

```text
[L1-5. Đọc ADC Soil sensor]
Đối tượng: Soil sensor + ADC ESP32
Công việc: Đọc tín hiệu analog từ cảm biến độ ẩm đất
[BIẾN VÀO] Không có
[BIẾN DÙNG] ADC_filtered
[BIẾN RA] ADC_filtered
Giải thích: ADC_filtered là giá trị ADC đã lọc nhiễu để tính H_soil.
```

v

```text
[L1-6. Tính H_soil]
Đối tượng: Task_Sensor
Công việc: Chuyển ADC_filtered sang phần trăm độ ẩm đất
[BIẾN VÀO] ADC_filtered
[BIẾN DÙNG] ADC_filtered, H_soil
[BIẾN RA] H_soil
Giải thích: H_soil là biến chính để Layer 2 chọn luật tưới.
```

v

```text
[L1-7. Kiểm tra lỗi Soil sensor]
Đối tượng: Task_Sensor
Công việc: Kiểm tra giá trị H_soil có hợp lệ không
[BIẾN VÀO] ADC_filtered, H_soil
[BIẾN DÙNG] Soil_status, Soil_Error_Flag, Error_Flag
[BIẾN RA] Soil_status, Soil_Error_Flag, Error_Flag
Giải thích: Nếu Soil sensor lỗi, Layer 2 không được tưới tự động dựa trên dữ liệu đất sai.
```

v

```text
[L1-8. Đóng gói SoilData]
Đối tượng: Task_Sensor
Công việc: Gói dữ liệu nhánh Soil sensor
[BIẾN VÀO] H_soil, Soil_status, Soil_Error_Flag, Error_Flag
[BIẾN DÙNG] SoilData
[BIẾN RA] SoilData
Giải thích: SoilData là dữ liệu trung gian trước khi ghép vào SensorData_t.
```

v

```text
[L1-9. Cập nhật timestamp]
Đối tượng: Task_Sensor
Công việc: Gắn thời điểm đo cho dữ liệu cảm biến
[BIẾN VÀO] Không có
[BIẾN DÙNG] timestamp
[BIẾN RA] timestamp
Giải thích: timestamp giúp Layer 2 và Layer 5 chọn bản dữ liệu mới nhất.
```

v

```text
[L1-10. Đóng gói SensorData_t]
Đối tượng: Task_Sensor
Công việc: Ghép DHTData và SoilData thành SensorData_t
[BIẾN VÀO] DHTData, SoilData, timestamp
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t là đầu ra chuẩn của Layer 1.
```

v

```text
[L1-11. Gửi SensorData_t sang Layer 2]
Đối tượng: Task_Sensor + sensorToControlQueue
Công việc: Gửi dữ liệu cảm biến cho Task_Control
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] SensorData_t, sensorToControlQueue
[BIẾN RA] SensorData_t
Giải thích: Layer 2 nhận SensorData_t tại L2-1 để xử lý điều khiển.
```

v

```text
[L1-12. Cập nhật SensorData_t mới nhất cho Layer 5]
Đối tượng: Task_Sensor + cloudTelemetryQueue hoặc shared latest buffer
Công việc: Cung cấp SensorData_t mới nhất cho Task_Cloud
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] SensorData_t, timestamp, cloudTelemetryQueue hoặc shared latest buffer
[BIẾN RA] SensorData_t
Giải thích: Task_Cloud lấy SensorData_t mới nhất theo timestamp; dữ liệu này có thể đến từ Layer 1 hoặc Layer 4.
```

---

# Mũi tên nối layer

```text
Layer 1 [L1-11. Gửi SensorData_t sang Layer 2]
-> RTOS [sensorToControlQueue]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```

```text
Layer 1 [L1-12. Cập nhật SensorData_t mới nhất cho Layer 5]
-> RTOS [cloudTelemetryQueue hoặc shared latest buffer]
-> Layer 5 [L5-1. Task_Cloud nhận SensorData_t]
Biến truyền: SensorData_t
Quy tắc chọn: dùng bản mới nhất theo timestamp.
```

