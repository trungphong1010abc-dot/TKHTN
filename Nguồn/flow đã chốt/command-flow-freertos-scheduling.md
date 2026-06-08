# Command flow - FreeRTOS Scheduling

File này giải thích thuật toán scheduling để đưa vào slide/báo cáo. Flow nối ô chi tiết nằm ở:

```text
command-flow-layer-rtos.md
```

---

# 1. FreeRTOS là gì trong hệ này

```text
FreeRTOS không phải một task.
FreeRTOS không phải layer nghiệp vụ.
FreeRTOS là runtime/scheduler nền cho Task_Sensor, Task_Control, Task_Actuator, Task_Feedback và Task_Cloud.
```

Cascading Model và Scheduler là hai ý khác nhau:

| Nội dung | Ý nghĩa |
|---|---|
| Cascading Model | Mô tả luồng dữ liệu: Sensor -> Control -> Actuator -> Feedback -> IoT |
| FreeRTOS Scheduler | Chọn task nào chạy trên CPU tại từng thời điểm |

---

# 2. Thuật toán scheduling dùng trong đồ án

| Kỹ thuật | Dùng để làm gì | Áp dụng trong hệ |
|---|---|---|
| Preemptive Scheduling | Task priority cao có thể ngắt task priority thấp | Task_Actuator/Task_Control không bị Task_Cloud làm chậm |
| RMS Priority Assignment | Gán priority theo chu kỳ/deadline | Task deadline gấp hơn được ưu tiên cao hơn |
| Round Robin / Time slicing | Chia CPU cho các task cùng priority | Task_Sensor và Task_Feedback cùng P3 nếu cùng READY |
| Queue blocking | Task chờ dữ liệu thay vì chiếm CPU | Task_Actuator chờ `actuatorCmdQueue` |
| Semaphore/Mutex | Báo hiệu hoặc bảo vệ shared buffer | Dùng nếu Layer 1/4 và Layer 5 cùng truy cập latest buffer |
| Watchdog Timer | Phát hiện task treo/quá hạn | Ưu tiên đưa bơm về trạng thái an toàn |

---

# 3. Priority đã chốt để trình bày

| Task | Priority | Kiểu chạy | Lý do |
|---|---:|---|---|
| Task_Actuator | 5 | Event-driven | Phản ứng nhanh với `pump_cmd` |
| Task_Control | 4 | Event-driven theo `SensorData_t` | Tạo quyết định tưới |
| Task_Sensor | 3 | Periodic | Đọc cảm biến định kỳ |
| Task_Feedback | 3 | Event-driven sau `pump_state` | Đo phản hồi sau tưới |
| Task_Cloud | 2 | Periodic/event-driven | Telemetry không critical bằng điều khiển |
| P1 | 1 | Chưa dùng | Dự phòng |
| Idle Task | 0 | Hệ thống | Task nền của FreeRTOS |

Ghi chú:

```text
Số priority chỉ là mức trình bày tương đối.
Trong code thật phải kiểm tra configMAX_PRIORITIES.
FreeRTOS ưu tiên số lớn hơn.
```

---

# 4. Cách scheduler chọn task

```text
1. Task chưa có dữ liệu hoặc đang delay nằm ở Waiting/Blocked.
2. Khi queue có dữ liệu hoặc hết delay, task chuyển sang READY.
3. Scheduler chọn task READY có priority cao nhất.
4. Nếu task priority cao hơn xuất hiện READY, nó preempt task priority thấp.
5. Nếu nhiều task cùng priority cùng READY, time slicing/Round Robin chia CPU theo tick.
6. Khi task xử lý xong, task gửi queue nếu cần rồi quay lại chờ queue hoặc delay.
```

Ví dụ:

```text
Task_Cloud đang Running.
Task_Control tạo pump_cmd và gửi vào actuatorCmdQueue.
Task_Actuator chuyển sang READY.
Vì Task_Actuator P5 cao hơn Task_Cloud P2, scheduler cho Task_Actuator chạy trước.
```

---

# 5. Tick rate giải thích dễ hiểu

```text
Tick rate là nhịp đồng hồ mà FreeRTOS dùng để đếm thời gian.
Nếu tick rate = 100 Hz thì 1 tick = 10 ms.
Nếu tick rate = 1000 Hz thì 1 tick = 1 ms.
```

Ý nghĩa:

```text
vTaskDelay và vTaskDelayUntil không delay trực tiếp theo ms, mà delay theo số tick.
Vì vậy khi code nên dùng pdMS_TO_TICKS(500) thay vì tự đổi 500 ms thành số tick.
```

Tradeoff:

```text
Tick rate cao hơn -> thời gian chia nhỏ hơn, delay mịn hơn.
Tick rate cao hơn -> scheduler bị gọi thường xuyên hơn, overhead cao hơn.
```

Trong flow hiện tại:

```text
Chưa chốt 100 Hz hay 1000 Hz nếu chưa có config FreeRTOS thật.
Flow chỉ ghi thời gian theo ms và khi code dùng pdMS_TO_TICKS().
```

---

# 6. Queue / Semaphore đọc như thế nào

| Cơ chế | Dùng khi nào | Có truyền dữ liệu không | Ví dụ |
|---|---|---|---|
| Queue | Cần gửi struct/lệnh giữa task | Có | `SensorData_t`, `ControlData_t`, `pump_cmd`, `pump_state` |
| Semaphore | Cần báo hiệu sự kiện | Không truyền struct | ISR báo task xử lý |
| Mutex | Cần bảo vệ vùng dữ liệu dùng chung | Không truyền struct | shared latest buffer |

Quy tắc:

```text
Queue dùng để truyền dữ liệu.
Semaphore/Mutex dùng để đồng bộ hoặc bảo vệ tài nguyên.
Không dùng semaphore thay queue khi cần truyền struct.
```

---

# 7. Watchdog Timer đề xuất

| Task | Timeout đề xuất | Khi timeout |
|---|---|---|
| Task_Actuator | Không vượt quá 1 chu kỳ phản ứng lệnh bơm | Ưu tiên trạng thái bơm an toàn |
| Task_Control | Không vượt quá 1 chu kỳ điều khiển | Không phát lệnh tưới mới |
| Task_Feedback | delay đất ổn định + đọc ADC/retry + margin | Không dùng phản hồi cũ để kết luận đất đủ ẩm |
| Task_Sensor | Không vượt quá 1 chu kỳ đo cảm biến | Xem dữ liệu chu kỳ đó không mới |
| Task_Cloud | 1 đến 2 chu kỳ telemetry | Bỏ/hoãn telemetry, không tác động pump_cmd |

Nguyên tắc:

```text
Task feed watchdog sau khi hoàn thành một vòng công việc hợp lệ.
Watchdog không thay Error_Flag cảm biến.
Watchdog không tạo biến mới trong SensorData_t, ControlData_t hoặc TelemetryPacket_t nếu chưa chốt.
```

