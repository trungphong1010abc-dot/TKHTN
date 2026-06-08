# Đề cương slide thuyết trình dự án

Trạng thái:

```text
Bản nháp chi tiết để tham khảo khi làm slide final.
```

Nguồn chính:

```text
../design-rules-bien-va-chu-ky.md
../tong-hop-flow-thong-nhat.md
../command-flow-layer1-sensing.md
../command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md
../command-flow-layer3-actuator.md
../command-flow-layer4-feedback.md
../command-flow-layer5-iot-communication.md
../command-flow-layer-rtos.md
../bang-schedule-rtos.md
```

---

# Cấu trúc đề xuất

```text
Slide 1. Tiêu đề
Slide 2. Mục lục
Slide 3. Bài toán
Slide 4. Kiến trúc phân tầng
Slide 5. Luồng dữ liệu chính
Slide 6. Layer 1 - Sensing
Slide 7. Layer 2 - Điều khiển tưới
Slide 8. GDD/CGDD và stage
Slide 9. Luật tưới và an toàn tưới
Slide 10. Layer 3 - Actuator
Slide 11. Layer 4 - Feedback
Slide 12. Layer 5 - IoT Communication
Slide 13. Trạng thái kết nối
Slide 14. RTOS trong hệ thống
Slide 15. Cascading Model và RTOS Scheduler
Slide 16. Priority và scheduling
Slide 17. Queue, Semaphore và Mutex
Slide 18. Watchdog Timer
Slide 19. Vòng điều khiển kín
Slide 20. Kết luận
```

---

# Nội dung cần nhấn mạnh

## Kiến trúc

```text
Layer 1 tạo SensorData_t.
Layer 2 quyết định tưới.
Layer 3 chấp hành bơm.
Layer 4 phản hồi sau tưới.
Layer 5 truyền telemetry.
RTOS là tầng nền, không phải Layer 6.
```

## Luồng dữ liệu

```text
SensorData_t:
Layer 1 hoặc Layer 4 tạo.

pump_cmd:
Layer 2 tạo, Layer 3 nhận.

pump_state:
Layer 3 tạo, Layer 4 nhận.

ControlData_t:
Layer 2 tạo, Layer 5 nhận.

TelemetryPacket_t:
Layer 5 tạo và publish MQTT.
```

## RTOS

```text
FreeRTOS quản lý task, priority, queue, semaphore/mutex, tick và watchdog.
FreeRTOS không tính GDD/CGDD.
FreeRTOS không tạo pump_cmd.
FreeRTOS không điều khiển GPIO.
FreeRTOS không tạo TelemetryPacket_t.
```

## Priority

```text
Task_Actuator P5
Task_Control  P4
Task_Sensor   P3
Task_Feedback P3
Task_Cloud    P2
P1 chưa dùng
P0 Idle Task
```

## Queue/Semaphore/Mutex

```text
Queue: truyền dữ liệu.
Semaphore: báo hiệu hoặc đồng bộ.
Mutex: bảo vệ tài nguyên dùng chung.
```

## Watchdog

```text
Watchdog phát hiện task treo/quá hạn.
Watchdog không quyết định tưới.
Watchdog không thêm biến mới vào struct.
```

---

# Câu nói kết luận

```text
Hệ thống được thiết kế theo kiến trúc phân tầng rõ ràng, xử lý điều khiển tại ESP32, có phản hồi sau tưới và dùng FreeRTOS để ưu tiên các task điều khiển bơm hơn task truyền telemetry.
```
