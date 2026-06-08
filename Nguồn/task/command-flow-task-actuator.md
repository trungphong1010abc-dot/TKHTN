# Command flow - Task_Actuator

Vị trí:

```text
Task_Actuator không phải layer riêng.
Task_Actuator là task FreeRTOS thực hiện Layer 3 - Actuator.
```

Luồng tổng quát:

```text
Layer 2 [Task_Control]
-> RTOS [actuatorCmdQueue]
-> Layer 3 [Task_Actuator]
-> GPIO ESP32
-> AO3400/relay
-> máy bơm
-> pump_state
-> RTOS [actuatorFeedbackQueue]
-> Layer 4 [Task_Feedback]
```

Biến:

```text
Đầu vào: pump_cmd
Đầu ra: pump_state
Queue nhận: actuatorCmdQueue
Queue gửi: actuatorFeedbackQueue
```

Không tự thêm:

```text
actuator_request
actuator_enable
actuator_status
motor_pwm
current_sensor
fault_code
```

---

# Flow chi tiết

```text
[TA-1. Chờ pump_cmd từ actuatorCmdQueue]
Đối tượng: Task_Actuator + FreeRTOS Queue
Công việc: Block chờ lệnh bơm từ Task_Control
[BIẾN VÀO] actuatorCmdQueue
[BIẾN DÙNG] xQueueReceive, pump_cmd, timeout hoặc portMAX_DELAY
[BIẾN RA] pump_cmd
Giải thích: Task_Actuator không tự tạo pump_cmd; pump_cmd do Layer 2 tạo.
```

v

```text
[TA-2. Kiểm tra pump_cmd]
Đối tượng: Task_Actuator
Công việc: Chọn nhánh bật hoặc tắt bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_cmd
Giải thích: pump_cmd = ON đi nhánh bật bơm; pump_cmd = OFF đi nhánh tắt bơm.
```

Nếu `pump_cmd = ON`:

```text
[TA-3A. Xuất GPIO bật bơm]
Đối tượng: GPIO ESP32
Công việc: Xuất tín hiệu điều khiển để AO3400/relay đóng nguồn cho bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] tín hiệu GPIO
Giải thích: ESP32 chỉ điều khiển tầng công suất, không cấp nguồn trực tiếp cho bơm.
```

v

```text
[TA-4A. Cập nhật pump_state = ON]
Đối tượng: Task_Actuator
Công việc: Ghi nhận trạng thái bơm đã bật
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: pump_state = ON dùng để báo cho Layer 4 biết bơm đã chấp hành.
```

Nếu `pump_cmd = OFF`:

```text
[TA-3B. Xuất GPIO tắt bơm]
Đối tượng: GPIO ESP32
Công việc: Xuất tín hiệu điều khiển để AO3400/relay ngắt nguồn cho bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] tín hiệu GPIO
Giải thích: Đây là nhánh an toàn khi cần dừng bơm.
```

v

```text
[TA-4B. Cập nhật pump_state = OFF]
Đối tượng: Task_Actuator
Công việc: Ghi nhận trạng thái bơm đã tắt
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: pump_state = OFF dùng để báo cho Layer 4 biết bơm đã dừng.
```

Sau hai nhánh:

```text
[TA-5. Gửi pump_state sang actuatorFeedbackQueue]
Đối tượng: Task_Actuator + FreeRTOS Queue
Công việc: Gửi trạng thái bơm cho Task_Feedback
[BIẾN VÀO] pump_state
[BIẾN DÙNG] xQueueSend, pump_state, actuatorFeedbackQueue
[BIẾN RA] pump_state
Giải thích: Layer 4 dùng pump_state để bắt đầu quy trình đo phản hồi H_soil.
```

v

```text
[TA-6. Quay lại chờ lệnh mới]
Đối tượng: Task_Actuator + FreeRTOS Scheduler
Công việc: Quay lại trạng thái blocked trên actuatorCmdQueue
[BIẾN VÀO] Không có
[BIẾN DÙNG] xQueueReceive
[BIẾN RA] Không có
Giải thích: Task_Actuator không chạy vòng lặp chiếm CPU; nó chờ queue.
```

---

# Mũi tên nối

```text
Layer 2 [L2-13 hoặc L2-15. Tạo pump_cmd]
-> RTOS [actuatorCmdQueue]
-> Task_Actuator [TA-1. Chờ pump_cmd từ actuatorCmdQueue]
Biến truyền: pump_cmd
```

```text
Task_Actuator [TA-5. Gửi pump_state sang actuatorFeedbackQueue]
-> RTOS [actuatorFeedbackQueue]
-> Layer 4 [L4-1. Nhận pump_state và chờ đất ổn định]
Biến truyền: pump_state
```

---

# Không được hiểu sai

```text
Task_Actuator không quyết định tưới.
Task_Actuator không tính WATER_DURATION_MS.
Task_Actuator không đọc Soil sensor.
Task_Actuator không tạo SensorData_t.
Task_Actuator không publish MQTT.
Task_Actuator chỉ nhận pump_cmd, điều khiển GPIO/AO3400/relay, rồi tạo pump_state.
```

