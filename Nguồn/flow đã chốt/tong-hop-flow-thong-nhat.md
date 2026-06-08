# Tổng hợp flow thống nhất

Mục đích:

```text
File này là mục lục thống nhất để đọc và vẽ lại toàn bộ hệ thống.
Thứ tự nghiệp vụ chính là Layer 1 -> Layer 2 -> Layer 3 -> Layer 4 -> Layer 5.
RTOS là tầng nền, không phải Layer 6.
```

---

# 1. Thứ tự flow chuẩn

| Thứ tự | Layer/File | Task chính | Vai trò | Đầu ra chính |
|---|---|---|---|---|
| 1 | `command-flow-layer1-sensing.md` | Task_Sensor | Đọc DHT22, Soil sensor, tạo dữ liệu cảm biến | SensorData_t |
| 2 | `command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md` | Task_Control | Tính GDD/CGDD, chọn stage, tạo quyết định tưới | pump_cmd, ControlData_t |
| 3 | `command-flow-layer3-actuator.md` | Task_Actuator | Xuất GPIO qua AO3400/relay để bật/tắt bơm | pump_state |
| 4 | `command-flow-layer4-feedback.md` | Task_Feedback | Đo lại H_soil sau tưới và gửi phản hồi | SensorData_t phản hồi |
| 5 | `command-flow-layer5-iot-communication.md` | Task_Cloud | Tạo TelemetryPacket_t và publish MQTT | TelemetryPacket_t |
| Nền | `command-flow-layer-rtos.md` | FreeRTOS | Scheduler, queue, semaphore/mutex, watchdog | Task chạy đúng ưu tiên |

---

# 2. Luồng dữ liệu chính

| Từ | Đến | Cơ chế | Biến truyền |
|---|---|---|---|
| Layer 1 | Layer 2 | sensorToControlQueue | SensorData_t |
| Layer 1 | Layer 5 | cloudTelemetryQueue hoặc shared latest buffer | SensorData_t |
| Layer 2 | Layer 3 | actuatorCmdQueue | pump_cmd |
| Layer 3 | Layer 4 | actuatorFeedbackQueue | pump_state |
| Layer 4 | Layer 2 | sensorToControlQueue | SensorData_t phản hồi |
| Layer 4 | Layer 5 | cloudTelemetryQueue hoặc shared latest buffer | SensorData_t phản hồi |
| Layer 2 | Layer 5 | controlToCloudQueue | ControlData_t |
| Layer 5 | ThingsBoard/Server | MQTT/JSON/TCP/IP/WiFi | TelemetryPacket_t |

---

# 3. Quy tắc đã chốt sau khi so sánh flow hiện tại và flow Phong gửi

```text
Giữ 5 layer nghiệp vụ.
Layer 4 là Feedback, không phải RTOS.
RTOS là tầng nền.
Task_Cloud thuộc Layer 5, không phải layer riêng.
Task_Feedback thuộc Layer 4, không có file flow riêng.
ThingsBoard/Server là endpoint ngoài ESP32.
SensorData_t vào Layer 5 lấy bản mới nhất theo timestamp, có thể từ Layer 1 hoặc Layer 4.
wifi_status sinh từ WiFi/Ethernet stack.
cloud_status sinh từ MQTT connect/publish result.
```

File phân tích so sánh:

```text
bang-tong-hop-toi-uu-flow-hien-tai-va-phong.md
```

---

# 4. Thư mục

```text
task/
```

Dùng cho flow task riêng nếu thật sự cần tách khỏi layer. Hiện có:

```text
task/command-flow-task-actuator.md
task/command-flow-task-cloud.md
task/flow actuator.png
```

```text
thuyết trình/
```

Dùng cho slide và báo cáo:

```text
thuyết trình/slide-thuyet-trinh-du-an-ban-duyet.md
thuyết trình/bao-cao-mau-du-an-ban-duyet.md
thuyết trình/de-cuong-slide-thuyet-trinh-du-an.md
```

---

# 5. Thứ tự đọc đề xuất

```text
1. design-rules-bien-va-chu-ky.md
2. bang-tong-hop-toi-uu-flow-hien-tai-va-phong.md
3. tong-hop-flow-thong-nhat.md
4. command-flow-layer1-sensing.md
5. command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md
6. command-flow-layer3-actuator.md
7. command-flow-layer4-feedback.md
8. command-flow-layer5-iot-communication.md
9. command-flow-layer-rtos.md
10. command-flow-freertos-scheduling.md
11. bang-schedule-rtos.md
```
