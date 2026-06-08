# Command flow - RTOS nền

Vai trò:

```text
RTOS không phải Layer 6 và không nằm sau Layer 5.
RTOS là tầng nền chạy bên dưới các task của Layer 1-5.
RTOS quản lý task, priority, queue, semaphore/mutex, tick và watchdog.
RTOS không tính GDD/CGDD, không tạo pump_cmd, không điều khiển GPIO, không publish MQTT.
```

Cách đặt trong sơ đồ:

```text
Layer 1 - Task_Sensor
Layer 2 - Task_Control
Layer 3 - Task_Actuator
Layer 4 - Task_Feedback
Layer 5 - Task_Cloud
        |
        v
RTOS nền - Scheduler + Queue + Semaphore/Mutex + Watchdog
        |
        v
ESP32 Hardware
```

---

# Đối tượng RTOS

Task:

```text
Task_Sensor
Task_Control
Task_Actuator
Task_Feedback
Task_Cloud
```

Queue/buffer:

```text
sensorToControlQueue
actuatorCmdQueue
actuatorFeedbackQueue
controlToCloudQueue
cloudTelemetryQueue hoặc shared latest buffer
```

Semaphore/Mutex:

```text
Dùng nếu có shared latest buffer hoặc tài nguyên dùng chung.
Queue dùng để truyền dữ liệu.
Semaphore/Mutex không dùng để thay Queue khi cần truyền struct.
```

Watchdog:

```text
Giám sát task treo/quá hạn.
Không thêm watchdog_status vào SensorData_t, ControlData_t hoặc TelemetryPacket_t nếu chưa chốt riêng.
```

---

# Priority đề xuất

| Task | Priority trình bày | Lý do |
|---|---:|---|
| Task_Actuator | 5 | Phản ứng nhanh với pump_cmd, liên quan trực tiếp bật/tắt bơm |
| Task_Control | 4 | Tạo quyết định tưới, pump_cmd và ControlData_t |
| Task_Sensor | 3 | Tạo SensorData_t định kỳ |
| Task_Feedback | 3 | Đo phản hồi H_soil sau tưới |
| Task_Cloud | 2 | Telemetry không được làm chậm điều khiển |
| Chưa dùng | 1 | Dự phòng cho background/log nếu sau này cần |
| Idle Task | 0 | Task hệ thống của FreeRTOS, không phải task nghiệp vụ |

Ghi chú:

```text
Đây là priority tương đối để trình bày thiết kế.
Khi code thật, priority phải nằm trong 0 đến configMAX_PRIORITIES - 1.
Số priority càng lớn thì ưu tiên càng cao.
Không bắt buộc phải dùng đủ mọi mức priority.
```

---

# Flow RTOS chi tiết

```text
[RTOS-1. Khởi tạo task]
Đối tượng: FreeRTOS
Công việc: Tạo Task_Sensor, Task_Control, Task_Actuator, Task_Feedback, Task_Cloud
[BIẾN VÀO] task function, stack_size, priority
[BIẾN DÙNG] xTaskCreate, TaskHandle_t, priority
[BIẾN RA] các task sẵn sàng cho scheduler
Giải thích: Task là đơn vị được FreeRTOS chọn để chạy trên CPU.
```

v

```text
[RTOS-2. Khởi tạo queue/buffer]
Đối tượng: FreeRTOS Queue
Công việc: Tạo các đường truyền dữ liệu giữa task
[BIẾN VÀO] kiểu dữ liệu, queue_length
[BIẾN DÙNG] xQueueCreate, QueueHandle_t
[BIẾN RA] sensorToControlQueue, actuatorCmdQueue, actuatorFeedbackQueue, controlToCloudQueue, cloudTelemetryQueue
Giải thích: Queue chỉ vận chuyển dữ liệu do task tạo ra, không sửa nội dung dữ liệu.
```

v

```text
[RTOS-3. Khởi tạo semaphore/mutex nếu cần]
Đối tượng: FreeRTOS Semaphore/Mutex
Công việc: Bảo vệ shared latest buffer hoặc tài nguyên dùng chung
[BIẾN VÀO] tài nguyên dùng chung
[BIẾN DÙNG] xSemaphoreCreateMutex, xSemaphoreTake, xSemaphoreGive
[BIẾN RA] Mutex/Semaphore handle
Giải thích: Chỉ dùng khi thật sự có truy cập chung; không dùng thay Queue để truyền struct.
```

v

```text
[RTOS-4. Gán priority]
Đối tượng: FreeRTOS Scheduler
Công việc: Gán ưu tiên theo RMS/criticality
[BIẾN VÀO] task set, chu kỳ, deadline, độ quan trọng
[BIẾN DÙNG] priority, configMAX_PRIORITIES
[BIẾN RA] bảng priority task
Giải thích: Task quan trọng hơn hoặc deadline gấp hơn có priority cao hơn.
```

v

```text
[RTOS-5. Start scheduler]
Đối tượng: FreeRTOS Scheduler
Công việc: Bắt đầu chọn task chạy trên CPU
[BIẾN VÀO] danh sách task, priority, trạng thái task
[BIẾN DÙNG] ready list, blocked list, tick interrupt
[BIẾN RA] task Running
Giải thích: Từ đây FreeRTOS quyết định task nào chạy, task nào chờ.
```

v

```text
[RTOS-6. Task chờ queue hoặc delay]
Đối tượng: Scheduler + Queue
Công việc: Đưa task chưa có dữ liệu hoặc chưa tới chu kỳ vào Waiting/Blocked
[BIẾN VÀO] xQueueReceive, vTaskDelay, vTaskDelayUntil
[BIẾN DÙNG] timeout, TickType_t
[BIẾN RA] task Waiting/Blocked
Giải thích: Task không được chạy vòng lặp chiếm CPU; khi chưa có việc thì phải chờ.
```

v

```text
[RTOS-7. Queue có dữ liệu thì đánh thức task đích]
Đối tượng: Queue
Công việc: Khi queue nhận dữ liệu, task đang chờ queue chuyển sang READY
[BIẾN VÀO] SensorData_t, ControlData_t, pump_cmd, pump_state
[BIẾN DÙNG] xQueueSend, xQueueReceive
[BIẾN RA] task đích READY
Giải thích: Queue vừa truyền dữ liệu vừa đánh thức task đích.
```

v

```text
[RTOS-8. Scheduler chọn task READY có priority cao nhất]
Đối tượng: FreeRTOS Scheduler
Công việc: Chọn task tiếp theo để chạy
[BIẾN VÀO] ready list, priority
[BIẾN DÙNG] priority-based scheduling
[BIẾN RA] task Running
Giải thích: Nếu Task_Actuator và Task_Cloud cùng READY thì Task_Actuator chạy trước.
```

v

```text
[RTOS-9. Preemption]
Đối tượng: FreeRTOS Scheduler
Công việc: Ngắt task priority thấp nếu task priority cao hơn chuyển sang READY
[BIẾN VÀO] task Running, task mới READY, priority
[BIẾN DÙNG] configUSE_PREEMPTION
[BIẾN RA] task priority cao hơn Running
Giải thích: Đây là lý do cloud không làm chậm lệnh bơm.
```

v

```text
[RTOS-10. Time slicing / Round Robin]
Đối tượng: FreeRTOS Scheduler
Công việc: Chia CPU theo tick cho các task cùng priority và cùng READY
[BIẾN VÀO] nhiều task cùng priority
[BIẾN DÙNG] configUSE_TIME_SLICING, tick
[BIẾN RA] các task cùng priority luân phiên chạy
Giải thích: Round Robin chỉ áp dụng cho task cùng priority, không thay thế priority scheduling.
```

v

```text
[RTOS-11. Watchdog giám sát task]
Đối tượng: Watchdog Timer
Công việc: Phát hiện task treo hoặc chạy quá hạn
[BIẾN VÀO] trạng thái hoàn thành của task, timeout theo task
[BIẾN DÙNG] feed watchdog, watchdog timeout
[BIẾN RA] phát hiện lỗi runtime nếu có
Giải thích: Watchdog không quyết định tưới; nó chỉ đưa hệ thống về hướng an toàn khi task lỗi.
```

---

# Bảng nối ô RTOS với 5 layer

| Luồng | Ô nguồn | Ô RTOS | Ô đích | Biến truyền | Cơ chế |
|---|---|---|---|---|---|
| Sensor sang Control | L1-11 | RTOS-7 -> RTOS-8 | L2-1 | SensorData_t | sensorToControlQueue |
| Sensor sang Cloud | L1-12 | RTOS-7 hoặc RTOS-3 | L5-1 | SensorData_t | cloudTelemetryQueue hoặc shared latest buffer |
| Control sang Actuator | L2-13/L2-15 | RTOS-7 -> RTOS-8 -> RTOS-9 | L3-1 | pump_cmd | actuatorCmdQueue |
| Actuator sang Feedback | L3-5 | RTOS-7 -> RTOS-8 | L4-1 | pump_state | actuatorFeedbackQueue |
| Feedback về Control | L4-7 | RTOS-7 -> RTOS-8 | L2-1 | SensorData_t phản hồi | sensorToControlQueue |
| Feedback sang Cloud | L4-8 | RTOS-7 hoặc RTOS-3 | L5-1 | SensorData_t phản hồi | cloudTelemetryQueue hoặc shared latest buffer |
| Control sang Cloud | L2-17 | RTOS-7 -> RTOS-8 | L5-2 | ControlData_t | controlToCloudQueue |
| Cloud publish | L5-3 -> L5-10 | Không qua queue RTOS nội bộ | ThingsBoard/Server | TelemetryPacket_t | MQTT/JSON/TCP/IP/WiFi |
| Watchdog | RTOS-11 | Giám sát song song | Các task | Không tạo biến nghiệp vụ mới | watchdog timeout |

---

# Câu bảo vệ ngắn

```text
Các layer tạo dữ liệu và xử lý thuật toán.
FreeRTOS chỉ điều phối task chạy trên CPU và truyền dữ liệu giữa task qua Queue.
Priority bảo đảm Task_Actuator và Task_Control không bị Task_Cloud làm chậm.
Watchdog phát hiện task treo/quá hạn và ưu tiên trạng thái an toàn.
```

