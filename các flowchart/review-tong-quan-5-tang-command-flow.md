# Design rules - Biến, chu kỳ và luật tưới đã chốt

Nguồn chốt biến:

```text
flow cảm biến DHT22.jpg
flow cảm biến điện dung.jpg
flow task control.jpg
flow task cloud (1).jpg
flow các layer.jpg
flow layer actuator.jpg
Tổng quan.pdf
```

## Luật bắt buộc

Không tự ý thêm biến mới vào flow, code mẫu, hoặc mô tả layer.

Mọi nội dung mô tả bằng tiếng Việt phải viết có dấu đầy đủ. Ngoại lệ duy nhất là tên biến, tên struct, tên task, tên file, tên hằng số và keyword code.

Không dùng khung gạch ASCII trong file flow. Ô flow phải viết dạng text sạch:

```text
[Tên ô]
Đối tượng: ...
Công việc: ...
[BIẾN VÀO] ...
[BIẾN DÙNG] ...
[BIẾN RA] ...
Giải thích: ...
```

Mũi tên hoặc hướng nối để ngoài ô, dùng:

```text
v
```

Nếu cần thêm biến mới, phải hỏi trước theo mẫu:

```text
Đề xuất thêm biến:
Tên biến:
Layer dùng:
Lý do cần:
Biến này thay thế hay bổ sung cho biến nào:
```

---

# 1. Layer 1 - Sensing Layer

## 1.1. Biến DHT22 đã chốt

```text
T_air
H_air
DHT_status
DHT_Error_Flag
Error_Flag
T_prev
H_prev
DHTData
```

## 1.2. Biến Soil sensor đã chốt

```text
ADC_filtered
H_soil
Soil_status
Soil_Error_Flag
Error_Flag
SoilData
```

## 1.3. Biến SensorData_t đã chốt

```text
SensorData_t
T_air
H_air
H_soil
DHT_status
Soil_status
Error_Flag
timestamp
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

Nếu tài liệu gốc ghi `Temp`, `RH`, `Soil%` thì khi đưa vào flow phải đổi về:

```text
Temp  -> T_air
RH    -> H_air
Soil% -> H_soil
```

---

# 2. Layer 2 - Edge Processing & Control Layer

## 2.1. Biến Layer 2 đã chốt

```text
SensorData_t
ControlData_t
T_air
H_air
H_soil
DHT_status
Soil_status
Error_Flag
data_valid
T_base
GDD_increment
GDD
CGDD
current_stage
soil_state
H_base
T_bonus
CGDD_bonus
H_threshold
last_watering_time
MIN_WATER_INTERVAL
pump_cmd
relay
pump_state
WATER_DURATION_MS
watering_time
control_status
timestamp
```

## 2.2. Luật GDD/CGDD

```text
GDD/CGDD dùng để xác định current_stage.
Theo Tổng quan.pdf, GDD/CGDD cập nhật cuối ngày để xác định Stage cho ngày hôm sau.
Nếu vẫn dùng flow task control hiện tại, GDD_increment và CGDD vẫn được giữ theo ảnh flow đã vẽ.
```

## 2.3. Luật điều khiển tưới chi tiết theo Stage

Layer 2 phải dùng `current_stage`, `H_soil`, `T_air`, `H_air` để chọn `soil_state`, `WATER_DURATION_MS`, `pump_cmd`.

Không tự đổi tên `WATER_DURATION_MS` thành `pumpTime`, `base_pumpTime`, `final_pumpTime`.

### Stage 1 - cây non

```text
H_soil > 70%
soil_state = quá ẩm
Hành động: không tưới, cảnh báo úng

55% <= H_soil <= 70%
soil_state = đủ ẩm
Hành động: không tưới

40% <= H_soil < 55%
soil_state = hơi khô
Hành động: tưới nhẹ 5s

25% <= H_soil < 40%
soil_state = khô
Hành động: tưới 8s

H_soil <= 25%
soil_state = rất khô
Hành động: tưới khẩn cấp 15s
```

Hiệu chỉnh thêm:

```text
T_air > 32°C -> +3s
H_air < 50% -> +5s nếu H_soil <= 25%
H_air > 90% -> -2s
```

### Stage 2 - cây phát triển mạnh

```text
H_soil > 75%
soil_state = quá ẩm
Hành động: không tưới, cảnh báo úng

45% <= H_soil <= 75%
soil_state = đủ ẩm
Hành động: không tưới

30% <= H_soil < 45%
soil_state = khô vừa
Hành động: tưới 10s

H_soil < 30%
soil_state = rất khô
Hành động: tưới mạnh 18s
```

Hiệu chỉnh thêm:

```text
T_air > 32°C -> +5s
H_air < 45% -> +5s
H_air > 90% -> -2s
```

### Stage 3 - cây trưởng thành

```text
H_soil > 80%
soil_state = quá ẩm
Hành động: khóa tưới, cảnh báo úng

40% <= H_soil <= 80%
soil_state = đủ ẩm
Hành động: không tưới

25% <= H_soil < 40%
soil_state = khô
Hành động: tưới 10s

H_soil < 25%
soil_state = rất khô
Hành động: tưới 20s
```

Hiệu chỉnh thêm:

```text
T_air > 35°C -> +5s
H_air < 45% -> +3s
H_air > 90% -> -2s
```

## 2.4. Quy tắc tạo lệnh bơm

```text
Nếu WATER_DURATION_MS > 0 và đã đủ MIN_WATER_INTERVAL:
pump_cmd = ON
control_status = WATERING

Nếu WATER_DURATION_MS = 0:
pump_cmd = OFF
control_status = SOIL_MOISTURE_OK hoặc SAFETY_LOCK hoặc SOIL_ERROR
```

## 2.5. Ngưỡng thời gian nghỉ tưới đã chốt

```text
MIN_WATER_INTERVAL = 30 phút
MIN_WATER_INTERVAL = 1800000 ms
```

Quy tắc:

```text
Chỉ cho phép tưới khi:
now - last_watering_time >= MIN_WATER_INTERVAL
```

Nếu chưa đủ 30 phút kể từ lần tưới gần nhất:

```text
pump_cmd = OFF
control_status = SAFETY_LOCK
```

---

# 3. Layer 3 - Actuator Layer

Theo ảnh `flow layer actuator.jpg` và `flow các layer.jpg`.

## 3.1. Biến Layer 3 đã chốt

```text
pump_cmd
pump_state
```

## 3.2. Thành phần phần cứng Layer 3

```text
GPIO ESP32
AO3400
relay
máy bơm
```

Layer 3 chỉ thực hiện:

```text
pump_cmd
-> GPIO ESP32
-> AO3400/relay
-> máy bơm ON/OFF
-> pump_state
```

Không tự thêm biến trung gian trong Layer 3:

```text
actuator_request
actuator_enable
gpio_state
driver_state
relay_control
pump_power_state
actuator_status
relay_state
watering_time
pumpTime
```

---

# 4. Bản đồ layer hiện tại

## 4.1. Các layer đã chốt về nội dung

```text
Layer 1 - Sensing Layer
Vai trò: Đọc DHT22 và Soil sensor, tạo SensorData_t.
Trạng thái file: Chưa có file command-flow riêng trong thư mục cac-flow-chart-da-tao.

Layer 2 - Edge Processing & Control Layer
Vai trò: Nhận SensorData_t, tính GDD/CGDD, xác định current_stage, chọn luật tưới, tạo pump_cmd và ControlData_t.
Trạng thái file: Đã có command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md.

Layer 3 - Actuator Layer
Vai trò: Nhận pump_cmd, điều khiển GPIO ESP32 -> AO3400/relay -> máy bơm, cập nhật pump_state.
Trạng thái file: Đã có command-flow-thuat-toan-tuoi-cay-layer3.md.

Layer 4 - FreeRTOS Scheduling & Inter-task Communication
Vai trò: Tổ chức task, queue, blocking, timeout và delay giữa Task_Control, Task_Actuator và Task_Cloud.
Trạng thái file: Đã có command-flow-layer4-freertos-scheduling.md.

Layer 5 - Task_Cloud
Vai trò: Nhận SensorData_t và ControlData_t, đóng gói TelemetryPacket_t.
Trạng thái file: Đã có command-flow-layer5-task-cloud.md.

Layer 6 - IoT Communication & Cloud Layer
Vai trò: Nhận TelemetryPacket_t, kiểm tra wifi_status và cloud_status, publish lên Cloud/Dashboard.
Trạng thái file: Đã có command-flow-layer6-iot-communication-cloud.md.
```

## 4.2. Các layer còn thiếu hoặc chưa chốt

```text
Layer 1 - Sensing Layer
Còn thiếu: File command-flow riêng.
Cần viết theo ảnh flow cảm biến DHT22.jpg, flow cảm biến điện dung.jpg và flow các layer.jpg.
```

## 4.3. Khối vật lý không viết thành command-flow layer riêng

```text
Đối tượng vật lý: đất + cây trồng
Vai trò: Nhận nước tưới, độ ẩm đất vùng rễ thay đổi, Soil sensor đo lại H_soil ở chu kỳ sau.
Không phải: task, queue, struct, tầng xử lý phần mềm, tầng truyền thông IoT.
Quy tắc: Không tạo biến mới như H_soil_after, SoilObjectData_t, water_input, crop_water_status nếu chưa được chốt.
```

Khối vật lý này chỉ được ghi như vòng phản hồi ngoài phần mềm:

```text
Layer 3 [Máy bơm ON/OFF]
-> Đối tượng vật lý [Đất + cây nhận nước]
-> Layer 1 [Soil sensor đo lại H_soil ở chu kỳ sau]
```

---

# 5. Layer 4 - FreeRTOS Scheduling & Inter-task Communication

Theo code mẫu `00_khoi_tao_queue_layer_flow.c`.

## 5.1. Đối tượng RTOS Layer 4

```text
Task_Control
Task_Actuator
Task_Cloud
actuatorCmdQueue
actuatorFeedbackQueue
controlToCloudQueue
```

Các task và queue này là thành phần RTOS nội bộ, không phải biến cảm biến hoặc biến quyết định tưới.

## 5.2. Kỹ thuật RTOS Layer 4

```text
xQueueCreate
xQueueSend
xQueueReceive
vTaskDelay
pdMS_TO_TICKS
portMAX_DELAY
timeout
blocking
```

Không chốt số priority cụ thể nếu code hoặc flow chưa có cấu hình priority rõ ràng.

## 5.3. Dữ liệu đi qua Layer 4

```text
pump_cmd
pump_state
ControlData_t
```

Layer 4 chỉ thực hiện:

```text
Layer 2 -> actuatorCmdQueue -> Layer 3:
pump_cmd

Layer 3 -> actuatorFeedbackQueue -> Layer 2:
pump_state

Layer 2 -> controlToCloudQueue -> Layer 5:
ControlData_t
```

Không đưa logic xử lý vào Layer 4:

```text
quyết định tưới
tính WATER_DURATION_MS
bật/tắt GPIO
đóng gói TelemetryPacket_t
MQTT publish
```

---

# 6. Layer 5 - Task_Cloud

## 6.1. Biến đầu vào của Task_Cloud

```text
SensorData_t
ControlData_t
```

## 6.2. Biến trong SensorData_t

```text
T_air
H_air
H_soil
DHT_status
Soil_status
Error_Flag
timestamp
```

## 6.3. Biến trong ControlData_t

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
Ảnh flow Task_Control dùng watering_time.
Ảnh flow các layer / ControlData_t có watering_duration.
Layer 5 giữ đúng tên watering_duration theo ControlData_t đã chốt.
```

## 6.4. Biến Layer 5 tạo ra

```text
TelemetryPacket_t
```

Layer 5 chỉ thực hiện:

```text
SensorData_t
ControlData_t
-> Task_Cloud
-> TelemetryPacket_t
-> Layer 6
```

Không đưa xử lý mạng vào Layer 5:

```text
MQTT publish
HTTP POST
wifi reconnect
cloud reconnect
```

---

# 7. Layer 6 - IoT Communication & Cloud Layer

Theo ảnh `các tầng khác chưa chọn flow.jpg`.

## 7.1. Biến Layer 6 đã chốt

```text
TelemetryPacket_t
wifi_status
cloud_status
```

Layer 6 chỉ thực hiện:

```text
TelemetryPacket_t
-> Task_Cloud + MQTT Client/publisher
-> Cloud/Dashboard
```

Không tự thêm:

```text
remote_config
ConfigData_t
dashboard_data
upload_status
cloud_ready
retry_count
device_id
schedule
auto_mode
manual_cmd
checksum
version
```

Ghi chú:

```text
Các biến hoặc chức năng cấu hình từ cloud như remote_config, ConfigData_t, auto_mode, manual_cmd chưa được chốt.
Nếu muốn thêm luồng cloud điều khiển ngược về Layer 2, phải đề xuất thêm biến trước.
```

---

# 8. Chu kỳ đã chốt

```text
Chu kỳ đo cảm biến / Task_Cloud: 60s
Delay ổn định Soil sensor: 500-1000 ms
Delay retry đọc ADC: 300-500 ms
Delay DHT22 ổn định: 2s
WATER_DURATION_MS
MIN_WATER_INTERVAL = 30 phút = 1800000 ms
last_watering_time
GDD/CGDD cập nhật cuối ngày theo Tổng quan.pdf
```

---

# 9. Nối biến giữa các layer đã chốt

```text
Layer 1 -> Layer 2:
SensorData_t

Layer 1 -> Layer 5:
SensorData_t

Layer 2 -> Layer 3:
Thông qua Layer 4 / actuatorCmdQueue
pump_cmd

Layer 3 -> Layer 2:
Thông qua Layer 4 / actuatorFeedbackQueue
pump_state

Layer 2 -> Layer 5:
Thông qua Layer 4 / controlToCloudQueue
ControlData_t

Layer 5 -> Layer 6:
TelemetryPacket_t

Layer 6 -> Cloud/Dashboard:
TelemetryPacket_t
```
