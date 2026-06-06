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

Trong mỗi file command flow của từng layer, phần đầu file phải có mục biến do layer đó tạo ra. Mục này chỉ được liệt kê các biến thật sự được layer đó tạo ra hoặc cập nhật, không trộn biến vào và biến dùng.

Mục biến tạo ra ở đầu file phải tách rõ:

```text
Biến tạo ra để gửi sang layer khác
Biến cập nhật nội bộ trong layer
```

Không được đặt danh sách biến tham chiếu hoặc danh sách biến dùng chung ở đầu file nếu danh sách đó làm lẫn với biến tạo ra.

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

# 4. Layer 4 - Feedback Layer

Theo ảnh `các tầng khác chưa chọn flow.jpg`, `flow cảm biến điện dung.jpg` và `flow các layer.jpg`.

## 4.1. Biến do Layer 4 tạo ra

```text
SensorData_t
```

Layer 4 là tầng phản hồi. Layer này đo lại độ ẩm đất sau tác động tưới và gửi dữ liệu phản hồi về controller cho chu kỳ sau.

## 4.2. Biến Layer 4 cập nhật nội bộ

```text
ADC_filtered
H_soil
Soil_status
Soil_Error_Flag
Error_Flag
timestamp
```

## 4.3. Thành phần Layer 4

```text
Soil sensor
ADC ESP32
ESP32
```

## 4.4. Luồng phản hồi Layer 4

```text
Layer 3
-> đất + cây nhận nước
-> Soil sensor đo lại H_soil
-> ESP32 đóng gói SensorData_t
-> Layer 2 xử lý ở chu kỳ sau
```

Layer 4 có thể dùng `pump_state` làm tín hiệu biết bơm đã chấp hành xong, nhưng không dùng `pump_state` để quyết định tưới.

Không tự thêm biến mới cho Layer 4:

```text
H_soil_after
SoilObjectData_t
water_input
water_absorbed
water_loss
soil_response_status
crop_water_status
root_zone_moisture
```

Không đưa logic xử lý vào Layer 4:

```text
quyết định tưới
tính WATER_DURATION_MS
điều khiển GPIO
MQTT publish
```

---

# 5. Layer RTOS - chưa đánh số lại

Layer RTOS/FreeRTOS Scheduling là cần thiết cho hệ thống nhưng chưa đặt số layer sau khi Layer 4 đã chốt là tầng phản hồi.

Các đối tượng RTOS đã có trong code mẫu:

```text
Task_Control
Task_Actuator
Task_Cloud
actuatorCmdQueue
actuatorFeedbackQueue
controlToCloudQueue
```

Các kỹ thuật RTOS:

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

RTOS chỉ tổ chức task và queue, không tạo biến tưới mới.

---

# 6. Chu kỳ đã chốt

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

# 7. Nối biến giữa các layer đã chốt

```text
Layer 1 -> Layer 2:
SensorData_t

Layer 2 -> Layer 3:
pump_cmd

Layer 3 -> Layer 4:
pump_state

Layer 4 -> Layer 2:
SensorData_t

Layer 2 -> Task_Cloud:
ControlData_t

Task_Cloud -> Cloud/Dashboard:
TelemetryPacket_t
```
