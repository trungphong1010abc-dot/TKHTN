# Design rules - Biến, chu kỳ và kiến trúc đã chốt

Mục đích:

```text
File này là nguồn chuẩn để viết flow, slide và báo cáo.
Nếu một file cũ hoặc flow của Phong mâu thuẫn với file này, dùng file này làm chuẩn.
```

---

# 1. Kiến trúc layer

```text
Hệ thống có 5 layer nghiệp vụ:
Layer 1 - Sensing
Layer 2 - GDD/CGDD Control
Layer 3 - Actuator
Layer 4 - Feedback
Layer 5 - IoT Communication
```

Quy tắc:

```text
RTOS là tầng nền, không phải Layer 6.
Layer 4 luôn là Feedback.
Task_Cloud là task thuộc Layer 5, không phải layer riêng.
Task_Feedback là task thực hiện Layer 4, không có flow riêng độc lập.
ThingsBoard/Server là endpoint ngoài ESP32, không phải task nội bộ.
```

---

# 2. Biến theo layer

## 2.1. Layer 1 - Sensing

```text
T_air
H_air
DHT_status
DHT_Error_Flag
ADC_filtered
H_soil
Soil_status
Soil_Error_Flag
Error_Flag
timestamp
DHTData
SoilData
SensorData_t
```

Không dùng tên thay thế:

```text
Temp
RH
Soil_percent
Soil%
Temperature
Humidity
```

Quy đổi tên nếu gặp tài liệu cũ:

```text
Temp  -> T_air
RH    -> H_air
Soil% -> H_soil
```

## 2.2. Layer 2 - GDD/CGDD Control

```text
SensorData_t
ControlData_t
data_valid
T_base
T_max
T_min
T_avg
GDD_daily
GDD
CGDD
current_stage
soil_state
H_threshold
WATER_DURATION_MS
MIN_WATER_INTERVAL
last_watering_time
watering_duration
pump_cmd
control_status
timestamp
control_mode
manual_cmd
```

Ghi chú:

```text
control_mode và manual_cmd chỉ dùng nếu có chức năng điều khiển thủ công từ ThingsBoard/Server.
Nếu chưa triển khai cloud command, mặc định control_mode = AUTO và không vẽ nhánh manual như luồng bắt buộc.
```

## 2.3. Layer 3 - Actuator

```text
pump_cmd
pump_state
```

Không tự thêm:

```text
actuator_request
actuator_enable
gpio_state
driver_state
relay_state
actuator_status
```

## 2.4. Layer 4 - Feedback

```text
pump_state
ADC_filtered
H_soil
Soil_status
Soil_Error_Flag
Error_Flag
timestamp
SensorData_t
```

Quy tắc:

```text
Layer 4 nhận pump_state từ Layer 3.
Layer 4 đo lại H_soil sau tưới.
Layer 4 tạo SensorData_t phản hồi.
Layer 4 không tạo pump_cmd và không tạo ControlData_t.
```

## 2.5. Layer 5 - IoT Communication

```text
SensorData_t
ControlData_t
TelemetryPacket_t
wifi_status
cloud_status
```

Nguồn biến:

```text
TelemetryPacket_t:
Do Task_Cloud tạo trong Layer 5 từ SensorData_t và ControlData_t.

wifi_status:
Do WiFi/Ethernet driver hoặc network stack của ESP32 cập nhật trong Layer 5.

cloud_status:
Do MQTT Client/publisher cập nhật từ kết quả connect/publish tới ThingsBoard/Server.
```

Không hiểu sai:

```text
wifi_status và cloud_status không đến từ Layer 1-4.
wifi_status và cloud_status không quyết định pump_cmd.
TelemetryPacket_t không phải dữ liệu server truyền vào ESP32.
```

---

# 3. Luồng biến giữa các layer

| Luồng | Biến truyền | Cơ chế |
|---|---|---|
| Layer 1 -> Layer 2 | SensorData_t | sensorToControlQueue |
| Layer 1 -> Layer 5 | SensorData_t | cloudTelemetryQueue hoặc shared latest buffer |
| Layer 2 -> Layer 3 | pump_cmd | actuatorCmdQueue |
| Layer 3 -> Layer 4 | pump_state | actuatorFeedbackQueue |
| Layer 4 -> Layer 2 | SensorData_t phản hồi | sensorToControlQueue |
| Layer 4 -> Layer 5 | SensorData_t phản hồi | cloudTelemetryQueue hoặc shared latest buffer |
| Layer 2 -> Layer 5 | ControlData_t | controlToCloudQueue |
| Layer 5 -> ThingsBoard/Server | TelemetryPacket_t | MQTT/JSON/TCP/IP/WiFi |

Quy tắc chọn `SensorData_t` cho Layer 5:

```text
Task_Cloud dùng SensorData_t mới nhất theo timestamp.
Bản này có thể đến từ Layer 1 hoặc Layer 4.
Không chọn theo tên layer, chọn theo timestamp.
```

---

# 4. Luật GDD/CGDD

```text
Trong ngày:
T_max = giá trị T_air lớn nhất trong ngày
T_min = giá trị T_air nhỏ nhất trong ngày
```

Khi chốt ngày:

```text
T_avg = (T_max + T_min) / 2
GDD_daily = T_avg - T_base
Nếu GDD_daily < 0 thì GDD_daily = 0
GDD = GDD_daily
CGDD = CGDD + GDD_daily
```

Quy tắc:

```text
CGDD chỉ cộng một lần khi chốt ngày.
Không cộng CGDD sau mỗi lần đọc cảm biến.
Nếu DHT_status lỗi, không cập nhật GDD/CGDD mới.
```

---

# 5. Luật tưới theo stage

## Stage 1 - Cây non

| Điều kiện H_soil | soil_state | WATER_DURATION_MS |
|---|---|---|
| H_soil > 70% | quá ẩm | 0 |
| 55% <= H_soil <= 70% | đủ ẩm | 0 |
| 40% <= H_soil < 55% | hơi khô | 5s |
| 25% <= H_soil < 40% | khô | 8s |
| H_soil <= 25% | rất khô | 15s |

Hiệu chỉnh:

```text
T_air > 32°C -> +3s
H_air < 50% -> +5s nếu H_soil <= 25%
H_air > 90% -> -2s
```

## Stage 2 - Cây phát triển mạnh

| Điều kiện H_soil | soil_state | WATER_DURATION_MS |
|---|---|---|
| H_soil > 75% | quá ẩm | 0 |
| 45% <= H_soil <= 75% | đủ ẩm | 0 |
| 30% <= H_soil < 45% | khô vừa | 10s |
| H_soil < 30% | rất khô | 18s |

Hiệu chỉnh:

```text
T_air > 32°C -> +5s
H_air < 45% -> +5s
H_air > 90% -> -2s
```

## Stage 3 - Cây trưởng thành

| Điều kiện H_soil | soil_state | WATER_DURATION_MS |
|---|---|---|
| H_soil > 80% | quá ẩm | 0 |
| 40% <= H_soil <= 80% | đủ ẩm | 0 |
| 25% <= H_soil < 40% | khô | 10s |
| H_soil < 25% | rất khô | 20s |

Hiệu chỉnh:

```text
T_air > 35°C -> +5s
H_air < 45% -> +3s
H_air > 90% -> -2s
```

---

# 6. Luật tạo pump_cmd

```text
Nếu Soil_status lỗi:
pump_cmd = OFF
control_status = SOIL_ERROR
```

```text
Nếu WATER_DURATION_MS = 0:
pump_cmd = OFF
control_status = SOIL_MOISTURE_OK hoặc SAFETY_LOCK hoặc cảnh báo quá ẩm
```

```text
Nếu WATER_DURATION_MS > 0 và đủ MIN_WATER_INTERVAL:
pump_cmd = ON
control_status = WATERING
```

```text
Nếu WATER_DURATION_MS > 0 nhưng chưa đủ MIN_WATER_INTERVAL:
pump_cmd = OFF
control_status = SAFETY_LOCK
```

Thời gian nghỉ tưới:

```text
MIN_WATER_INTERVAL = 30 phút = 1800000 ms
Điều kiện tưới: now - last_watering_time >= MIN_WATER_INTERVAL
```

---

# 7. Chu kỳ và delay

```text
Chu kỳ đo cảm biến mặc định: 60s
Chu kỳ telemetry mặc định: 60s
Delay ổn định Soil sensor sau tưới: 500-1000 ms
Delay retry ADC: 300-500 ms
Delay DHT22 ổn định: 2s
```

Quy tắc code:

```text
Trong flow ghi thời gian theo ms/s/phút.
Khi code FreeRTOS dùng pdMS_TO_TICKS().
Chưa tự chốt configTICK_RATE_HZ nếu chưa có config thật.
```

---

# 8. RTOS đã chốt

Kỹ thuật dùng:

```text
Preemptive Scheduling
RMS Priority Assignment
Round Robin/time slicing cho task cùng priority
Queue
Semaphore/Mutex nếu có shared buffer
Watchdog Timer
```

Priority trình bày:

| Task | Priority |
|---|---:|
| Task_Actuator | 5 |
| Task_Control | 4 |
| Task_Sensor | 3 |
| Task_Feedback | 3 |
| Task_Cloud | 2 |
| Chưa dùng | 1 |
| Idle Task FreeRTOS | 0 |

Ghi chú:

```text
Priority trên là tương đối để trình bày.
Khi code thật phải kiểm tra configMAX_PRIORITIES.
Số lớn hơn là ưu tiên cao hơn.
```

Queue/Semaphore:

```text
Queue dùng để truyền dữ liệu.
Semaphore dùng để báo hiệu.
Mutex dùng để bảo vệ vùng dữ liệu dùng chung.
Semaphore/Mutex không thay Queue khi cần truyền struct.
```

Watchdog:

```text
Watchdog phát hiện task treo/quá hạn.
Watchdog không quyết định tưới.
Watchdog không tạo pump_cmd.
Watchdog không thêm biến mới vào SensorData_t, ControlData_t hoặc TelemetryPacket_t nếu chưa chốt riêng.
```

---

# 9. Không được tự ý thêm biến

Nếu cần thêm biến mới, phải ghi đề xuất:

```text
Đề xuất thêm biến:
Tên biến:
Layer dùng:
Lý do cần:
Biến này thay thế hay bổ sung cho biến nào:
```

---

# 10. Các vấn đề mới đã cập nhật sau khi rà flow

## 10.1. Không dùng lại thiết kế cũ của Phong nếu mâu thuẫn bản chốt

```text
Thư mục flow phong gửi chỉ là nguồn tham khảo.
Không copy nguyên các flow trong đó vào bản chính nếu nội dung mâu thuẫn với 5 layer đã chốt.
```

Các điểm không lấy từ flow cũ:

```text
Không gọi RTOS là Layer 4.
Không tách Task_Cloud thành layer riêng.
Không vẽ Cloud/Dashboard như một ô xử lý nội bộ.
Không để Layer 3 nhảy thẳng sang RTOS rồi bỏ qua Feedback.
Không để pump_state quay trực tiếp về Layer 2 để đóng gói ControlData_t.
```

## 10.2. Thứ tự flow chuẩn để vẽ lại

```text
Layer 1 - Sensing
-> Layer 2 - GDD/CGDD Control
-> Layer 3 - Actuator
-> Layer 4 - Feedback
-> Layer 5 - IoT Communication
```

RTOS vẽ riêng:

```text
RTOS nền nằm dưới các task.
RTOS nối dữ liệu bằng Queue/Semaphore/Mutex.
RTOS không đứng trong chuỗi layer nghiệp vụ 1-5.
```

## 10.3. Quy tắc file và thư mục

```text
Các command-flow layer chính để ở thư mục gốc.
Các flow task phụ trợ để trong thư mục task/.
Slide, đề cương và báo cáo để trong thư mục thuyết trình/.
Không để file command-flow task trong thư mục thuyết trình/.
```

File chuẩn hiện tại:

```text
command-flow-layer1-sensing.md
command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md
command-flow-layer3-actuator.md
command-flow-layer4-feedback.md
command-flow-layer5-iot-communication.md
command-flow-layer-rtos.md
command-flow-freertos-scheduling.md
```

Task phụ trợ hiện tại:

```text
task/command-flow-task-actuator.md
task/command-flow-task-cloud.md
```

## 10.4. Feedback chỉ có một flow

```text
Layer 4 Feedback là flow duy nhất cho phản hồi.
Task_Feedback chỉ là task thực hiện Layer 4.
Không tạo thêm command-flow-task-feedback.md ở thư mục gốc hoặc trong task/.
```

Mũi tên đúng:

```text
Layer 3 [pump_state]
-> RTOS [actuatorFeedbackQueue]
-> Layer 4 [Task_Feedback]
-> SensorData_t phản hồi
-> Layer 2 hoặc Layer 5
```

## 10.5. Task_Cloud nằm trong Layer 5

```text
Task_Cloud là task thuộc Layer 5.
Task_Cloud nhận SensorData_t và ControlData_t.
Task_Cloud tạo TelemetryPacket_t.
Layer 5 publish TelemetryPacket_t qua MQTT/JSON/TCP/IP/WiFi tới ThingsBoard/Server.
```

Không viết:

```text
Task_Cloud -> Layer 5 như hai tầng tách biệt.
Layer 5 nhận TelemetryPacket_t từ một layer Task_Cloud riêng.
Cloud/Dashboard là một ô xử lý trong ESP32.
```

## 10.6. ThingsBoard/Server chỉ là endpoint

```text
ThingsBoard/Server là hệ thống bên ngoài ESP32.
Nó nhận telemetry qua MQTT, lưu và hiển thị dữ liệu.
Nó không phải task FreeRTOS.
Nó không phải layer xử lý nội bộ.
```

Khi vẽ:

```text
Layer 5 [L5-10. ThingsBoard/Server nhận telemetry]
Biến vào: TelemetryPacket_t qua MQTT
```

Không vẽ thêm:

```text
Layer 5 -> Cloud/Dashboard -> một layer mới
```

## 10.7. Quy tắc SensorData_t vào Layer 5

```text
Task_Cloud lấy SensorData_t mới nhất theo timestamp.
SensorData_t có thể đến từ Layer 1 hoặc Layer 4.
Layer 1 là dữ liệu đo định kỳ.
Layer 4 là dữ liệu phản hồi sau tưới.
```

Không hỏi theo kiểu:

```text
Task_Cloud lấy từ Layer 1 hay Layer 4 theo thời gian à?
```

Câu trả lời chuẩn:

```text
Task_Cloud không chọn theo tên layer.
Task_Cloud chọn bản SensorData_t mới nhất theo timestamp.
```

## 10.8. Cloud lỗi không được làm sai điều khiển cục bộ

```text
Nếu mất WiFi hoặc server lỗi, Layer 2/3/4 vẫn tự điều khiển tưới tại ESP32.
Task_Cloud có thể bỏ/hoãn telemetry.
Task_Cloud không được tác động pump_cmd.
```

## 10.9. RTOS scheduling phải tách khỏi Cascading Model

```text
Cascading Model mô tả luồng xử lý dữ liệu.
FreeRTOS Scheduler mô tả task nào chạy trên CPU.
Hai phần này liên quan nhưng không phải một.
```

Khi trình bày:

```text
Phần kiến trúc phần mềm:
Cascading Model: Sensor -> Control -> Actuator -> Feedback -> IoT

Phần RTOS:
Preemptive Scheduling + RMS + Round Robin/time slicing + Queue + Semaphore/Mutex + Watchdog
```

## 10.10. Timeline RTOS không phải bảng biến

```text
Timeline RTOS chỉ cho thấy Waiting, Ready, Running và task nào đang chạy.
Không đặt SensorData_t, pump_cmd, pump_state lên trục timeline như thể chúng là biến của scheduler.
Các biến truyền phải trình bày ở bảng Queue/Semaphore riêng.
```

