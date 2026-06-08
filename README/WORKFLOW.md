# Workflow ngắn gọn

## 1. Khởi tạo
- `setup()`:
  - bật Serial
  - khởi động watchdog
  - gọi `sensingBegin()`, `controlBegin()`, `actuatorBegin()`, `cloudBegin()`
  - tạo các queue và mutex
  - tạo 5 task FreeRTOS

## 2. Task chính
- `Task_Sensor`:
  - đọc DHT và cảm biến đất
  - gửi dữ liệu tới `sensorToControlQueue` và `cloudTelemetryQueue`
  - cập nhật dữ liệu sensor mới nhất

- `Task_Control`:
  - nhận dữ liệu từ `sensorToControlQueue`
  - tính GDD/CGDD và giai đoạn sinh trưởng
  - xác định `soil_state`, `WATER_DURATION_MS`, `control_status`
  - nếu cần tưới: gửi lệnh ON rồi OFF sau thời gian tưới
  - gửi kết quả điều khiển tới `controlToCloudQueue`

- `Task_Actuator`:
  - nhận lệnh bơm từ `actuatorCmdQueue`
  - bật/tắt chân GPIO điều khiển bơm
  - gửi trạng thái bơm về `actuatorFeedbackQueue`

- `Task_Feedback`:
  - nhận trạng thái bơm
  - nếu bơm tắt, chờ đất ổn định
  - đọc lại dữ liệu đất và gửi về control + cloud

- `Task_Cloud`:
  - giữ kết nối Wi-Fi/MQTT
  - lấy dữ liệu sensor và control mới nhất
  - gửi telemetry lên MQTT
  - in log trạng thái lên Serial

## 3. Loop
- `loop()` chỉ ngủ 1 giây vì toàn bộ công việc đã ở task.

## 4. Dòng chảy tóm tắt
1. Sensor đo dữ liệu.
2. Control nhận, quyết định tưới.
3. Actuator bật/tắt bơm.
4. Feedback đọc lại sau khi tắt bơm.
5. Cloud gửi dữ liệu lên server.
