# Slide thuyết trình dự án - bản duyệt nội dung

Tên đề tài:

```text
Hệ thống tưới cây thông minh sử dụng ESP32, FreeRTOS và IoT
```

Môn học:

```text
Mạng cảm biến không dây
```

Mục đích file:

```text
Bản nội dung slide để đọc và duyệt trước khi làm bản trình chiếu final.
```

---

# Slide 1. Tiêu đề

Nội dung:

```text
Hệ thống tưới cây thông minh sử dụng ESP32, FreeRTOS và IoT

Thành phần chính:
ESP32, DHT22, Soil sensor, bơm nước, FreeRTOS, MQTT/ThingsBoard.
```

Lời nói:

```text
Dự án xây dựng hệ thống tưới cây tự động dựa trên cảm biến, xử lý tại ESP32, điều khiển bơm, đo phản hồi sau tưới và gửi telemetry lên server IoT.
```

---

# Slide 2. Mục lục

Nội dung:

```text
1. Bài toán và mục tiêu
2. Kiến trúc phân tầng
3. Luồng dữ liệu chính
4. Thuật toán điều khiển tưới
5. Actuator và Feedback
6. IoT Communication
7. FreeRTOS Scheduling
8. Queue, Semaphore, Watchdog
9. Kết luận
```

---

# Slide 3. Bài toán

Nội dung:

```text
Bài toán:
Tự động quyết định tưới dựa trên nhiệt độ, độ ẩm không khí, độ ẩm đất và giai đoạn sinh trưởng.

Yêu cầu:
- Đọc dữ liệu cảm biến.
- Tính toán quyết định tưới.
- Điều khiển bơm.
- Đo phản hồi sau tưới.
- Gửi telemetry lên ThingsBoard/Server.
- Vẫn điều khiển tại ESP32 khi mất WiFi/server.
```

Lời nói:

```text
Hệ thống không chỉ bật/tắt bơm theo ngưỡng đơn giản, mà có xử lý theo stage cây, kiểm tra an toàn tưới và phản hồi sau tác động.
```

---

# Slide 4. Kiến trúc phân tầng

Nội dung:

```text
Layer 1 - Sensing:
Đọc DHT22 và Soil sensor, tạo SensorData_t.

Layer 2 - Edge Processing & Control:
Tính GDD/CGDD, xác định stage, tạo pump_cmd và ControlData_t.

Layer 3 - Actuator:
Nhận pump_cmd, điều khiển GPIO/relay/bơm, tạo pump_state.

Layer 4 - Feedback:
Đo lại H_soil sau tưới, tạo SensorData_t phản hồi.

Layer 5 - IoT Communication:
Task_Cloud tạo TelemetryPacket_t và gửi ThingsBoard/Server.

Layer RTOS:
Tầng nền điều phối task, queue, priority, semaphore/mutex và watchdog.
```

Lời nói:

```text
RTOS không phải Layer 6. RTOS nằm dưới các task để điều phối việc chạy trên CPU.
```

---

# Slide 5. Luồng dữ liệu chính

Nội dung:

```text
SensorData_t:
Layer 1 hoặc Layer 4 tạo.

ControlData_t:
Layer 2 tạo.

pump_cmd:
Layer 2 tạo, Layer 3 nhận.

pump_state:
Layer 3 tạo, Layer 4 nhận.

TelemetryPacket_t:
Layer 5 tạo và publish MQTT.
```

Lời nói:

```text
Mỗi biến có nơi tạo rõ ràng. RTOS chỉ truyền dữ liệu qua queue, không tự tạo biến nghiệp vụ.
```

---

# Slide 6. Layer 1 - Sensing

Nội dung:

```text
Đầu vào:
DHT22, Soil sensor.

Xử lý:
- Đọc T_air, H_air.
- Đọc ADC Soil sensor.
- Tính H_soil.
- Kiểm tra DHT_status, Soil_status, Error_Flag.
- Cập nhật timestamp.
- Đóng gói SensorData_t.

Đầu ra:
SensorData_t.
```

Flow tham chiếu:

```text
../command-flow-layer1-sensing.md
```

---

# Slide 7. Layer 2 - Điều khiển tưới

Nội dung:

```text
Input:
SensorData_t.

Các bước:
1. Kiểm tra dữ liệu cảm biến.
2. Tính/cập nhật GDD và CGDD.
3. Xác định current_stage.
4. Phân loại soil_state.
5. Chọn WATER_DURATION_MS.
6. Hiệu chỉnh theo T_air và H_air.
7. Kiểm tra MIN_WATER_INTERVAL.
8. Tạo pump_cmd và ControlData_t.
```

Lời nói:

```text
Layer 2 là nơi duy nhất quyết định tưới. Layer 3/4/5 không tự quyết định tưới.
```

Flow tham chiếu:

```text
../command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md
```

---

# Slide 8. GDD/CGDD và stage

Nội dung:

```text
GDD:
Đơn vị nhiệt tích lũy trong ngày.

CGDD:
Tổng GDD cộng dồn theo thời gian.

current_stage:
Giai đoạn sinh trưởng xác định từ CGDD.

Vai trò:
Stage quyết định ngưỡng H_soil và thời gian tưới phù hợp.
```

---

# Slide 9. Luật tưới và an toàn tưới

Nội dung:

```text
Layer 2 dùng:
current_stage + H_soil + T_air + H_air
-> soil_state
-> WATER_DURATION_MS
-> pump_cmd.

MIN_WATER_INTERVAL = 30 phút = 1800000 ms.

Nếu chưa đủ thời gian nghỉ:
pump_cmd = OFF
control_status = SAFETY_LOCK.
```

---

# Slide 10. Layer 3 - Actuator

Nội dung:

```text
Input:
pump_cmd.

Xử lý:
pump_cmd -> GPIO ESP32 -> AO3400/relay -> máy bơm.

Output:
pump_state.

Không làm:
- Không tính GDD/CGDD.
- Không tính WATER_DURATION_MS.
- Không quyết định tưới.
- Không publish MQTT.
```

Flow tham chiếu:

```text
../command-flow-layer3-actuator.md
../task/command-flow-task-actuator.md
```

---

# Slide 11. Layer 4 - Feedback

Nội dung:

```text
Input:
pump_state.

Xử lý:
- Nhận pump_state từ actuatorFeedbackQueue.
- Chờ đất ổn định sau tưới.
- Đọc lại Soil sensor.
- Tính lại H_soil.
- Cập nhật timestamp.
- Đóng gói SensorData_t phản hồi.
- Gửi về Layer 2 và cập nhật cho Layer 5.

Output:
SensorData_t phản hồi.
```

Lời nói:

```text
Task_Feedback nằm trong Layer 4, không còn file flow riêng.
```

Flow tham chiếu:

```text
../command-flow-layer4-feedback.md
```

---

# Slide 12. Layer 5 - IoT Communication

Nội dung:

```text
Input:
SensorData_t mới nhất theo timestamp.
ControlData_t mới nhất.

Xử lý:
Task_Cloud -> TelemetryPacket_t -> MQTT/JSON -> TCP -> IP -> WiFi/Ethernet.

Output:
Telemetry lên ThingsBoard/Server.
```

Flow tham chiếu:

```text
../command-flow-layer5-iot-communication.md
```

---

# Slide 13. Trạng thái kết nối

Nội dung:

```text
wifi_status:
Lấy từ WiFi/Ethernet driver hoặc network stack.

cloud_status:
Lấy từ kết quả connect/publish của MQTT Client.

Quy tắc:
cloud_status không quyết định pump_cmd.
Mất WiFi/server thì ESP32 vẫn tự điều khiển tưới.
```

---

# Slide 14. RTOS trong hệ thống

Nội dung:

```text
FreeRTOS không phải task.
FreeRTOS không phải Layer 6.
FreeRTOS là tầng nền runtime/scheduler.

RTOS quản lý:
- Task.
- Priority.
- Queue.
- Semaphore/Mutex.
- Tick.
- Watchdog.
```

Flow tham chiếu:

```text
../command-flow-layer-rtos.md
```

---

# Slide 15. Cascading Model và RTOS Scheduler

Nội dung:

```text
Cascading Model:
Task_Sensor -> Task_Control -> Task_Cloud.

Ý nghĩa:
Mô tả luồng xử lý dữ liệu.

RTOS Scheduler:
Preemptive Scheduling + RMS + Queue + Semaphore + Watchdog.

Ý nghĩa:
Mô tả task nào được chạy trên CPU.
```

Lời nói:

```text
Cascading Model không phải RTOS Scheduler.
```

---

# Slide 16. Priority và scheduling

Nội dung:

```text
Task_Actuator P5
Task_Control  P4
Task_Sensor   P3
Task_Feedback P3
Task_Cloud    P2
P1 chưa dùng trong thiết kế hiện tại
P0 Idle Task của FreeRTOS

Nguyên tắc:
Priority hợp lệ từ 0 đến configMAX_PRIORITIES - 1.
Số càng lớn thì ưu tiên càng cao.
```

Hình nên dùng:

```text
../timeline-task-rtos-clean.png
```

---

# Slide 17. Queue, Semaphore và Mutex

Nội dung:

```text
Queue:
Truyền dữ liệu giữa task.

Semaphore:
Báo hiệu hoặc đồng bộ sự kiện.

Mutex:
Bảo vệ tài nguyên dùng chung.

Các queue chính:
sensorToControlQueue
actuatorCmdQueue
actuatorFeedbackQueue
controlToCloudQueue
cloudTelemetryQueue hoặc shared latest buffer.
```

Lời nói:

```text
Queue giống như gửi dữ liệu. Semaphore giống như báo hiệu. Mutex giống như khóa bảo vệ vùng dữ liệu chung.
```

---

# Slide 18. Watchdog Timer

Nội dung:

```text
Vai trò:
Phát hiện task bị treo hoặc chạy quá hạn.

Áp dụng cho:
Task_Sensor
Task_Control
Task_Actuator
Task_Feedback
Task_Cloud

Nguyên tắc:
Task điều khiển và actuator có timeout ngắn hơn task cloud.
Task_Cloud timeout không được làm dừng điều khiển tại ESP32.
Khi lỗi control/actuator, ưu tiên trạng thái bơm an toàn.
```

---

# Slide 19. Vòng điều khiển kín

Nội dung:

```text
Sensor đo môi trường
-> Layer 2 quyết định tưới
-> Layer 3 chấp hành bơm
-> Layer 4 đo phản hồi H_soil
-> Layer 2 xử lý chu kỳ sau
-> Layer 5 gửi telemetry
```

---

# Slide 20. Kết luận

Nội dung:

```text
Hệ thống được thiết kế theo kiến trúc phân tầng rõ ràng.

Layer 2 là nơi quyết định tưới.
Layer 3 chấp hành.
Layer 4 phản hồi.
Layer 5 truyền thông IoT.
RTOS điều phối task và ưu tiên điều khiển bơm hơn telemetry.

Ưu tiên thiết kế:
An toàn tưới và bảo vệ cây quan trọng hơn dashboard realtime.
```

---

# Phần cần duyệt

```text
1. Tên đề tài final có giữ như slide 1 không?
2. Số slide 20 có vừa thời lượng thuyết trình không?
3. Có cần thêm slide bảng luật tưới Stage 1/2/3 chi tiết không?
4. Có cần xuất sang PPTX/HTML sau khi duyệt nội dung không?
```
