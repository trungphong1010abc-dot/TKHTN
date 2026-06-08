# Command flow - Layer 4 Feedback

Vai trò:

```text
Layer 4 là tầng phản hồi của hệ điều khiển kín.
Task_Feedback nằm trong Layer 4, không có flow riêng độc lập.
Layer 4 nhận pump_state, chờ đất ổn định, đo lại H_soil và tạo SensorData_t phản hồi.
```

Đối tượng:

```text
Task_Feedback
Soil sensor điện dung
ADC ESP32
actuatorFeedbackQueue
sensorToControlQueue
cloudTelemetryQueue hoặc shared latest buffer
```

Biến:

```text
Đầu vào: pump_state
Đầu ra: SensorData_t phản hồi
```

---

# Flow chi tiết

```text
[Layer 4. Feedback Layer]
Đối tượng: Task_Feedback + Soil sensor + ADC ESP32
Công việc: Đọc lại độ ẩm đất sau khi bơm đã chấp hành
[BIẾN VÀO] pump_state
[BIẾN DÙNG] ADC_filtered, H_soil, Soil_status, Soil_Error_Flag, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: Layer 4 phản ánh tác động thật của tưới lên đất để Layer 2 có dữ liệu mới cho chu kỳ sau.
```

v

```text
[L4-1. Nhận pump_state và chờ đất ổn định]
Đối tượng: Task_Feedback + actuatorFeedbackQueue
Công việc: Nhận trạng thái bơm rồi chờ nước thấm vào vùng cảm biến
[BIẾN VÀO] pump_state
[BIẾN DÙNG] pump_state, actuatorFeedbackQueue, vTaskDelay, pdMS_TO_TICKS
[BIẾN RA] pump_state
Giải thích: Task_Feedback không tự biết bơm vừa chạy; nó nhận pump_state từ Task_Actuator.
```

v

```text
[L4-2. Đọc lại ADC Soil sensor]
Đối tượng: Soil sensor + ADC ESP32
Công việc: Đọc tín hiệu độ ẩm đất sau tưới
[BIẾN VÀO] Không có
[BIẾN DÙNG] ADC_filtered
[BIẾN RA] ADC_filtered
Giải thích: Đây là lần đo phản hồi sau tác động của Layer 3.
```

v

```text
[L4-3. Tính lại H_soil]
Đối tượng: Task_Feedback
Công việc: Chuyển ADC_filtered thành phần trăm độ ẩm đất
[BIẾN VÀO] ADC_filtered
[BIẾN DÙNG] ADC_filtered, H_soil
[BIẾN RA] H_soil
Giải thích: H_soil mới cho biết đất đã đủ ẩm hay vẫn khô sau tưới.
```

v

```text
[L4-4. Kiểm tra Soil_status]
Đối tượng: Task_Feedback
Công việc: Kiểm tra giá trị soil phản hồi có hợp lệ không
[BIẾN VÀO] ADC_filtered, H_soil
[BIẾN DÙNG] Soil_status, Soil_Error_Flag, Error_Flag
[BIẾN RA] Soil_status, Soil_Error_Flag, Error_Flag
Giải thích: Nếu cảm biến lỗi, Layer 2 không được dùng dữ liệu phản hồi này để khẳng định đất đã đủ ẩm.
```

v

```text
[L4-5. Cập nhật timestamp phản hồi]
Đối tượng: Task_Feedback
Công việc: Gắn thời điểm đo phản hồi
[BIẾN VÀO] Không có
[BIẾN DÙNG] timestamp
[BIẾN RA] timestamp
Giải thích: timestamp giúp Layer 2 và Layer 5 chọn bản SensorData_t mới nhất.
```

v

```text
[L4-6. Đóng gói SensorData_t phản hồi]
Đối tượng: Task_Feedback
Công việc: Đóng gói dữ liệu phản hồi sau tưới
[BIẾN VÀO] H_soil, Soil_status, Soil_Error_Flag, Error_Flag, timestamp
[BIẾN DÙNG] SensorData_t
[BIẾN RA] SensorData_t
Giải thích: SensorData_t phản hồi dùng cùng kiểu dữ liệu với SensorData_t của Layer 1, nhưng timestamp mới hơn.
```

v

```text
[L4-7. Gửi SensorData_t phản hồi về Layer 2]
Đối tượng: Task_Feedback + sensorToControlQueue
Công việc: Gửi dữ liệu phản hồi cho Task_Control
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] SensorData_t, sensorToControlQueue
[BIẾN RA] SensorData_t
Giải thích: Layer 2 nhận lại SensorData_t ở L2-1 và xử lý như dữ liệu mới.
```

v

```text
[L4-8. Cập nhật SensorData_t mới nhất cho Layer 5]
Đối tượng: Task_Feedback + cloudTelemetryQueue hoặc shared latest buffer
Công việc: Cung cấp dữ liệu phản hồi cho Task_Cloud
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] SensorData_t, timestamp, cloudTelemetryQueue hoặc shared latest buffer
[BIẾN RA] SensorData_t
Giải thích: Nếu bản phản hồi mới hơn bản từ Layer 1, Task_Cloud dùng bản này để đóng telemetry.
```

---

# Mũi tên nối layer

```text
Layer 3 [L3-5. Gửi pump_state sang Layer 4]
-> RTOS [actuatorFeedbackQueue]
-> Layer 4 [L4-1. Nhận pump_state và chờ đất ổn định]
Biến truyền: pump_state
```

```text
Layer 4 [L4-7. Gửi SensorData_t phản hồi về Layer 2]
-> RTOS [sensorToControlQueue]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t phản hồi
```

```text
Layer 4 [L4-8. Cập nhật SensorData_t mới nhất cho Layer 5]
-> RTOS [cloudTelemetryQueue hoặc shared latest buffer]
-> Layer 5 [L5-1. Task_Cloud nhận SensorData_t]
Biến truyền: SensorData_t phản hồi
Quy tắc chọn: dùng bản mới nhất theo timestamp.
```

