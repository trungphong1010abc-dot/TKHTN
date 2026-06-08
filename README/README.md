# ESP32 Irrigation System - Giải thích mã nguồn

## 1. Tổng quan

Đây là một hệ thống tưới tự động chạy trên ESP32. Mã nguồn được chia thành các phần:

- `src/main.cpp`: khởi tạo toàn bộ hệ thống, tạo các hàng đợi và tác vụ (FreeRTOS tasks).
- `src/sensing.cpp`: đọc dữ liệu cảm biến môi trường gồm DHT và cảm biến độ ẩm đất.
- `src/control.cpp`: xử lý quyết định tưới nước, tính GDD/CGDD và chọn thời lượng tưới.
- `src/actuator.cpp`: điều khiển bơm nước qua một chân GPIO.
- `src/cloud.cpp`: kết nối Wi-Fi và gửi dữ liệu lên MQTT/ThingsBoard.
- `include/types.h`: định nghĩa cấu trúc dữ liệu, trạng thái và lời giúp chuyển enum thành văn bản.

## 2. Luồng dữ liệu chính

### 2.1. Tác vụ cảm biến - `Task_Sensor`

- Chạy tuần hoàn mỗi `SENSOR_PERIOD_MS`.
- Đọc:
  - `T_air` và `H_air` từ cảm biến DHT.
  - `ADC_filtered` và `H_soil` từ cảm biến độ ẩm đất.
- Gói dữ liệu vào `SensorData_t` và gửi tới:
  - hàng đợi `sensorToControlQueue` để điều khiển.
  - hàng đợi `cloudTelemetryQueue` để gửi cloud.
- Đồng thời cập nhật bản sao dữ liệu mới nhất trong `latestSensorData`.

### 2.2. Tác vụ điều khiển - `Task_Control`

- Nhận dữ liệu từ `sensorToControlQueue`.
- Gọi `processControlData(SensorData)` để quyết định:
  - bơm có cần bật không.
  - trạng thái đất (`soil_state`).
  - thời lượng tưới `WATER_DURATION_MS`.
  - trạng thái điều khiển `control_status`.
- Nếu cần bật bơm:
  - gửi lệnh ON vào `actuatorCmdQueue`.
  - chờ đúng `WATER_DURATION_MS`.
  - gửi lệnh OFF và báo `WATERING_DONE`.
- Nếu không cần bật bơm, gửi OFF luôn và báo `SOIL_MOISTURE_OK` hoặc `TOO_WET`.
- Kết quả điều khiển cũng được gửi vào `controlToCloudQueue`.

### 2.3. Tác vụ mạch bơm - `Task_Actuator`

- Nhận lệnh `PumpCmd_t` từ `actuatorCmdQueue`.
- Gọi `applyPumpCmd(command)` để thay đổi trạng thái chân GPIO của bơm.
- Ghi lại trạng thái bơm (`PumpState_t`) và gửi vào `actuatorFeedbackQueue`.

### 2.4. Tác vụ phản hồi - `Task_Feedback`

- Nhận trạng thái bơm từ `actuatorFeedbackQueue`.
- Nếu bơm đã tắt, chờ `SOIL_SETTLE_DELAY_MS` cho đất ổn định.
- Đọc lại cảm biến đất nhanh bằng `readFeedbackSensorData(getLatestSensorData())`.
- Gửi lại dữ liệu phản hồi này về điều khiển và cloud.

### 2.5. Tác vụ cloud - `Task_Cloud`

- Chạy vòng lặp kiểm tra Wi-Fi/MQTT liên tục.
- Thu thập dữ liệu sensor và điều khiển mới nhất từ hai hàng đợi:
  - `cloudTelemetryQueue`
  - `controlToCloudQueue`
- Tạo `TelemetryPacket_t` gồm sensor, control, và trạng thái bơm.
- Gửi telemetry qua MQTT đến topic `v1/devices/me/telemetry`.
- In log trạng thái lên serial.

## 3. Các trạng thái và dữ liệu quan trọng

### 3.1. `SensorData_t`

- `dhtData`: chứa `T_air`, `H_air`, trạng thái DHT và flag lỗi.
- `soilData`: chứa `ADC_filtered`, `H_soil`, trạng thái đất và flag lỗi.
- `Error_Flag`: tổng hợp lỗi nếu bất kỳ cảm biến nào báo sai.
- `timestamp`: thời điểm đọc cảm biến.

### 3.2. `ControlData_t`

- `data_valid`: dữ liệu hợp lệ nếu cả DHT và đất đều OK.
- `T_max`, `T_min`, `T_avg`: nhiệt độ cực đại/ít nhất/trung bình dùng để tính GDD.
- `GDD`: Growing Degree Days trong ngày.
- `CGDD`: Cumulative GDD tích lũy trong chu kỳ.
- `current_stage`: giai đoạn sinh trưởng 1/2/3 dựa vào CGDD.
- `soil_state`: mô tả mức ẩm của đất như `too_wet`, `enough`, `dry`, `very_dry`, `slightly_dry`.
- `H_threshold`: ngưỡng ẩm tương ứng với `soil_state`.
- `WATER_DURATION_MS`: thời lượng mà hệ thống dự định bật bơm.
- `watering_duration`: thời lượng thực tế dùng trong quá trình tưới.
- `pump_cmd`: lệnh bơm ON/OFF.
- `control_status`: trạng thái điều khiển như `SOIL_ERROR`, `WATERING`, `SAFETY_LOCK`, `WATERING_DONE`.
- `timestamp`: thời điểm xử lý điều khiển.

### 3.3. `PumpState_t`

- `pump_state`: ON hoặc OFF.
- `timestamp`: thời điểm thay đổi bơm.

### 3.4. `TelemetryPacket_t`

- Gồm toàn bộ `SensorData`, `ControlData`, `PumpStateData` và trạng thái Wi-Fi/MQTT.

## 4. Cách `control.cpp` quyết định tưới

### 4.1. Tính GDD/CGDD

- Mỗi lần đo nhiệt độ, nếu DHT hợp lệ thì `updateGDD(T_air)` được gọi.
- Hệ thống thu thập `T_max` và `T_min` của ngày.
- Khi sang ngày mới hoặc đến 23:59, nó tính:
  - `T_avg = (T_max + T_min) / 2`
  - `GDD = max(0, T_avg - T_base)`
  - `CGDD += GDD`
- Dựa vào `CGDD`, `current_stage` được chọn:
  - stage 1 nếu nhỏ hơn `STAGE_2_CGDD`
  - stage 2 nếu nhỏ hơn `STAGE_3_CGDD`
  - stage 3 nếu lớn hoặc bằng `STAGE_3_CGDD`

### 4.2. Chọn quy tắc nước

`chooseWaterRule()` dùng `current_stage` và độ ẩm đất `H_soil` để gán:

- `soil_state`: mức ẩm
- `H_threshold`: ngưỡng ẩm tương ứng
- `WATER_DURATION_MS`: thời lượng tưới dự kiến

Ví dụ:
- Stage 1:
  - `H_soil >= 55` → `enough`, không tưới.
  - `40 <= H_soil < 55` → `slightly_dry`, tưới 5 giây.
  - `25 <= H_soil < 40` → `dry`, tưới 8 giây.
  - `H_soil < 25` → `very_dry`, tưới 15 giây.

Các ngưỡng khác nhau cho stage 2 và 3.

### 4.3. Điều chỉnh thời lượng theo khí hậu

Hàm `adjustWaterDuration()` điều chỉnh thêm hoặc bớt thời gian tưới dựa trên:

- `T_air` quá cao: tăng thời lượng.
- `H_air` quá thấp: tăng thời lượng.
- `H_air` quá cao: giảm thời lượng.

### 4.4. Khóa an toàn

Nếu đã có lần tưới gần đây và thời gian giữa hai lần chưa đủ `MIN_WATER_INTERVAL`, thì hệ thống sẽ:

- không bật bơm
- đặt `control_status = SAFETY_LOCK`

## 5. Bơm và chờ phản hồi

- `Task_Actuator` điều khiển chân GPIO bơm.
- Bơm được bật/tắt ngay khi nhận lệnh.
- `Task_Feedback` đảm bảo đọc lại cảm biến đất sau khi bơm tắt và đất ổn định, giúp hệ thống học lại về trạng thái mới của đất.

## 6. Kết nối cloud

- `cloudBegin()` thiết lập MQTT server từ `config.h`.
- `cloudLoop()` giữ kết nối Wi-Fi và MQTT.
- `publishTelemetryPacket()` tạo JSON và gửi đến ThingsBoard.
- Dữ liệu gửi đi gồm:
  - nhiệt độ, độ ẩm không khí, độ ẩm đất
  - trạng thái cảm biến
  - GDD, CGDD, current_stage
  - `soil_state`, `H_threshold`, `WATER_DURATION_MS`
  - `pump_cmd`, `pump_state`, `control_status`

## 7. `main.cpp` tổ chức hệ thống

### `setup()`

- Khởi động Serial.
- Khởi động watchdog ESP32.
- Gọi `sensingBegin()`, `controlBegin()`, `actuatorBegin()`, `cloudBegin()`.
- Tạo các hàng đợi FreeRTOS và mutex.
- Tạo 5 tác vụ:
  - `Task_Actuator`
  - `Task_Control`
  - `Task_Sensor`
  - `Task_Feedback`
  - `Task_Cloud`

### `loop()`

- Rỗng, chỉ `vTaskDelay(1000)` vì toàn bộ công việc được xử lý qua các tác vụ.

## 8. Ghi chú quan trọng

- Mã hiện tại là vận hành tự động.
- Không có cơ chế `setMode` hay `setWater` trong code này; nếu cần thêm RPC thì phải mở rộng `cloud.cpp` và thêm xử lý lệnh MQTT.
- `control_mode` không được định nghĩa trong dữ liệu điều khiển hiện tại; việc điều khiển chỉ là `auto` mặc định.

## 9. Nếu em cần giải thích từng file cụ thể

- `src/sensing.cpp`: cách đọc DHT và ADC, xử lý giá trị lỗi.
- `src/control.cpp`: thể hiện toàn bộ logic tưới và GDD.
- `src/actuator.cpp`: chuyển lệnh ON/OFF thành GPIO.
- `src/cloud.cpp`: tạo JSON telemetry và gửi MQTT.
- `src/main.cpp`: tổ chức các tác vụ và dữ liệu qua queue.

---

Nếu em muốn, mình có thể tiếp tục thêm phần "sơ đồ luồng dữ liệu" đơn giản bằng text hoặc mô tả dạng flowchart để trình bày cho đồ án.