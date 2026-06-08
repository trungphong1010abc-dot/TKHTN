# Bảng tổng hợp tối ưu flow hiện tại và flow Phong gửi

Mục đích file này:

```text
Đọc song song flow hiện tại và thư mục "flow phong gửi".
Chọn phần đúng, bỏ phần gây lệch thiết kế, rồi chốt thành một bảng thống nhất để vẽ lại flow.
```

Quy tắc chốt:

```text
Flow nghiệp vụ chính chỉ có 5 layer:
Layer 1 -> Layer 2 -> Layer 3 -> Layer 4 -> Layer 5

RTOS là tầng nền chạy bên dưới các task, không đánh số thành Layer 6.
Task_Cloud là task thuộc Layer 5 IoT Communication, không phải một layer riêng.
ThingsBoard/Server là điểm nhận bên ngoài, không phải một ô xử lý nội bộ của ESP32.
```

---

# 1. Bảng so sánh và quyết định tối ưu

| Hạng mục | Flow hiện tại | Flow Phong gửi | Vấn đề khi ghép thẳng | Bản tối ưu chốt |
|---|---|---|---|---|
| Thứ tự layer | Đã chốt 5 layer: Sensing, Control, Actuator, Feedback, IoT Communication | Có tài liệu cũ vẫn xem RTOS là Layer 4 và Cloud là Layer 5 | Nếu ghép thẳng sẽ làm mất Layer Feedback hoặc đẩy IoT sai vị trí | Giữ 5 layer hiện tại: L1 Sensing, L2 Control, L3 Actuator, L4 Feedback, L5 IoT Communication |
| Layer 1 - Sensing | Có `command-flow-layer1-sensing.md`, tạo `SensorData_t` | Có flow sensing khá chi tiết, đúng hướng | Không có xung đột lớn | Giữ flow hiện tại, có thể tham khảo cách Phong chia DHT22/Soil để giải thích rõ hơn |
| Layer 2 - GDD/CGDD Control | Có flow chi tiết hơn, gồm validate, GDD/CGDD, stage, luật tưới, `pump_cmd`, `ControlData_t` | Có flow ngắn hơn, có nhắc `manual_cmd`, `control_mode` | Phong có hướng cloud điều khiển ngược, cần tách rõ phần này không làm sai logic tự động | Giữ flow hiện tại làm chuẩn; nếu dùng manual/cloud command thì ghi là đầu vào tùy chọn của Layer 2, không để cloud thay thế thuật toán an toàn |
| Layer 3 - Actuator | Đã đổi thành `command-flow-layer3-actuator.md`, nhận `pump_cmd`, xuất GPIO và `pump_state` | Có flow actuator tương tự | Phong có đoạn cũ nối `pump_state` quay về Layer 2 để đóng gói cloud, dễ gây nhầm | Chốt: `pump_state` đi sang Layer 4 Feedback qua `actuatorFeedbackQueue`; `ControlData_t` do Layer 2 tạo riêng |
| Layer 4 - Feedback | Đã chốt là tầng phản hồi, nhận `pump_state`, đo lại đất, tạo `SensorData_t` phản hồi | Có flow feedback nhưng ngắn hơn | Bản Phong thiếu nhánh gửi dữ liệu phản hồi về Layer 2 và cập nhật dữ liệu mới nhất cho Layer 5 | Giữ flow hiện tại: Layer 4 gửi `SensorData_t` phản hồi về Layer 2 và cập nhật latest data cho Task_Cloud |
| Layer 5 - IoT Communication | Đã chốt Task_Cloud nằm trong Layer 5, tạo `TelemetryPacket_t`, gửi MQTT/TCP/IP/WiFi tới ThingsBoard/Server | Có flow IoT + Cloud nhưng dùng tên `Cloud/Dashboard`, có xu hướng coi cloud là một ô trong flow | Dễ làm người đọc tưởng dashboard là module trong ESP32 | Chốt: Layer 5 kết thúc ở publish MQTT; ThingsBoard/Server chỉ là endpoint nhận và hiển thị |
| RTOS | Có file riêng `command-flow-layer-rtos.md`, là tầng nền | Có tài liệu cũ đưa RTOS vào Layer 4 | Nếu đánh số RTOS thành layer sẽ phá thứ tự 5 layer | Chốt: RTOS là nền scheduling, queue, semaphore, watchdog; không đánh số trong flow nghiệp vụ |
| Task_Cloud | Là task thuộc Layer 5 | Có nơi tách Task_Cloud thành file/layer riêng | Làm sai bản chất: Task_Cloud không phải tầng độc lập | Chốt: Task_Cloud vẽ giống Task_Sensor, Task_Control, Task_Actuator; thuộc Layer 5 |
| Task_Feedback | Đã gộp vào `command-flow-layer4-feedback.md` | Có thể tách thành flow riêng | Trùng nội dung với Layer 4 | Chốt: Không dùng file task feedback riêng; Layer 4 chính là flow của Task_Feedback |
| `TelemetryPacket_t` | Do Task_Cloud tạo trong Layer 5 | Phong cũng có ý này | Nếu vẽ `TelemetryPacket_t` truyền từ server vào ESP32 là sai | Chốt: `SensorData_t` + `ControlData_t` -> Task_Cloud -> `TelemetryPacket_t` -> MQTT publish |
| `wifi_status` | Sinh từ WiFi/network stack trong Layer 5 | Có xuất hiện trong flow Phong | Nếu không ghi nguồn sẽ giống biến tự dưng xuất hiện | Chốt: Ghi chú trong ô Layer 5: `wifi_status` lấy từ kết quả kết nối WiFi/Ethernet |
| `cloud_status` | Sinh từ MQTT/server publish result trong Layer 5 | Có xuất hiện trong flow Phong | Nếu không ghi nguồn sẽ gây hỏi "cloud_status đâu ra" | Chốt: Ghi chú trong ô Layer 5: `cloud_status` lấy từ trạng thái kết nối MQTT hoặc kết quả publish |
| Cloud/Dashboard | Hiện đã đổi sang ThingsBoard/Server endpoint | Phong dùng `Cloud/Dashboard` | Tên này làm mơ hồ, không biết là task hay server ngoài | Chốt: Dùng `ThingsBoard/Server`; không xem là layer nội bộ |
| Giao tiếp task | Có Queue/Semaphore trong RTOS | Phong có hướng queue nhưng chưa thống nhất | Dễ lẫn queue truyền struct với semaphore báo hiệu | Chốt: Queue truyền dữ liệu; semaphore chỉ báo hiệu hoặc bảo vệ tài nguyên nếu thật sự dùng |

---

# 2. Bảng flow thống nhất cuối cùng để vẽ lại

| Thứ tự | File chuẩn | Đối tượng chính | Đầu vào | Công việc chính | Đầu ra | Nối sang |
|---|---|---|---|---|---|---|
| 1 | `command-flow-layer1-sensing.md` | `Task_Sensor`, DHT22, Soil sensor | Tín hiệu cảm biến | Đọc DHT22, đọc soil ADC, lọc, kiểm tra lỗi, đóng gói dữ liệu đo | `SensorData_t` | Layer 2 qua `sensorToControlQueue`; Layer 5 qua latest buffer hoặc `cloudTelemetryQueue` |
| 2 | `command-flow-layer2-thuat-toan-tuoi-gdd-cgdd.md` | `Task_Control` | `SensorData_t` mới nhất theo `timestamp` | Validate dữ liệu, tính GDD/CGDD, xác định `current_stage`, chọn luật tưới, tính `WATER_DURATION_MS`, tạo lệnh bơm | `pump_cmd`, `ControlData_t` | Layer 3 qua `actuatorCmdQueue`; Layer 5 qua `controlToCloudQueue` |
| 3 | `command-flow-layer3-actuator.md` | `Task_Actuator`, GPIO, AO3400/relay, máy bơm | `pump_cmd` | Nhận lệnh bơm, xuất GPIO, bật/tắt driver/relay/máy bơm, cập nhật trạng thái thực thi | `pump_state` | Layer 4 qua `actuatorFeedbackQueue` |
| 4 | `command-flow-layer4-feedback.md` | `Task_Feedback`, Soil sensor | `pump_state` | Chờ đất ổn định sau tưới, đọc lại soil sensor, tính lại `H_soil`, đóng gói dữ liệu phản hồi | `SensorData_t` phản hồi | Layer 2 qua `sensorToControlQueue`; Layer 5 qua latest buffer hoặc `cloudTelemetryQueue` |
| 5 | `command-flow-layer5-iot-communication.md` | `Task_Cloud`, MQTT client, network stack | `SensorData_t`, `ControlData_t` | Chọn dữ liệu mới nhất, tạo `TelemetryPacket_t`, chuyển JSON/MQTT, kiểm tra WiFi/server, publish telemetry | `TelemetryPacket_t`, `wifi_status`, `cloud_status` | ThingsBoard/Server |
| Nền | `command-flow-layer-rtos.md` | FreeRTOS Scheduler | Task, Queue, Semaphore, Watchdog | Tạo task, gán priority, scheduling, queue routing, timeout, watchdog | Task chạy đúng chu kỳ và đúng ưu tiên | Nằm dưới toàn bộ 5 layer |

---

# 3. Bảng đường nối dữ liệu sau khi tối ưu

| Từ | Đến | Cơ chế | Dữ liệu | Ý nghĩa |
|---|---|---|---|---|
| Layer 1 | Layer 2 | Queue | `SensorData_t` | Dữ liệu cảm biến dùng cho thuật toán điều khiển |
| Layer 1 | Layer 5 | Latest buffer hoặc Queue | `SensorData_t` | Dữ liệu cảm biến mới nhất dùng cho telemetry |
| Layer 2 | Layer 3 | Queue | `pump_cmd` | Lệnh bật/tắt bơm |
| Layer 2 | Layer 5 | Queue | `ControlData_t` | Trạng thái điều khiển, thời gian tưới, trạng thái bơm để gửi cloud |
| Layer 3 | Layer 4 | Queue | `pump_state` | Xác nhận actuator đã bật/tắt bơm |
| Layer 4 | Layer 2 | Queue | `SensorData_t` phản hồi | Dữ liệu độ ẩm đất sau tưới để điều khiển lại nếu cần |
| Layer 4 | Layer 5 | Latest buffer hoặc Queue | `SensorData_t` phản hồi mới nhất | Telemetry phản ánh trạng thái sau tưới |
| Layer 5 | ThingsBoard/Server | MQTT/TCP/IP/WiFi | `TelemetryPacket_t` | Gửi dữ liệu lên server để lưu và hiển thị |

---

# 4. Bảng phần nên lấy từ Phong và phần không nên lấy

| Nội dung trong flow Phong | Quyết định | Lý do |
|---|---|---|
| Cách chia DHT22 và Soil sensor trong Layer 1 | Nên lấy ý tưởng giải thích | Phù hợp với flow cảm biến, giúp Layer 1 dễ đọc hơn |
| `SensorData_t` gồm nhiệt độ, độ ẩm không khí, độ ẩm đất, trạng thái lỗi, timestamp | Giữ | Trùng với biến đã chốt |
| Layer 2 có GDD/CGDD, stage, luật tưới | Giữ nhưng dùng bản hiện tại chi tiết hơn | Bản hiện tại đã giải thích đầy đủ hơn |
| Layer 3 nhận `pump_cmd` và xuất `pump_state` | Giữ | Đúng chức năng actuator |
| Layer 4 là Feedback | Giữ | Đúng với thiết kế đã chốt sau Layer 3 |
| Task_Cloud tạo `TelemetryPacket_t` | Giữ | Đúng: telemetry packet được tạo ở Layer 5 |
| Gọi RTOS là Layer 4 | Không lấy | Layer 4 hiện đã chốt là Feedback; RTOS là tầng nền |
| Tách Task_Cloud thành layer riêng | Không lấy | Task_Cloud chỉ là task trong Layer 5 |
| Vẽ `Cloud/Dashboard` như một ô xử lý ngang hàng layer | Không lấy | Cloud là hệ thống ngoài ESP32, chỉ nhận telemetry |
| Để `pump_state` quay về Layer 2 để đóng gói `ControlData_t` một cách trực tiếp | Không lấy | `pump_state` phải qua Layer 4 Feedback; `ControlData_t` do Layer 2 tạo từ logic điều khiển và trạng thái mới nhất |
| Gộp IoT Communication và Cloud thành một layer nội bộ | Không lấy nguyên bản | Giữ phần IoT Communication, nhưng tách rõ ThingsBoard/Server là endpoint ngoài |

---

# 5. Bảng vấn đề cần sửa khi vẽ lại

| Vấn đề | Cách sửa khi vẽ |
|---|---|
| Sau Layer 3 lại nhảy sang RTOS | Không vẽ như vậy. Sau Layer 3 phải là Layer 4 Feedback. RTOS vẽ riêng bên dưới hoặc bên cạnh như tầng nền |
| Có hai flow feedback | Chỉ dùng `command-flow-layer4-feedback.md`; không tạo file task feedback riêng |
| Task_Cloud bị vẽ như layer riêng | Đặt Task_Cloud trong Layer 5, tương tự Task_Sensor nằm trong Layer 1 và Task_Control nằm trong Layer 2 |
| `TelemetryPacket_t` không nối vào ô nào | Vẽ rõ: `SensorData_t` + `ControlData_t` -> Task_Cloud -> tạo `TelemetryPacket_t` -> MQTT publish |
| `wifi_status` và `cloud_status` tự nhiên xuất hiện | Ghi chú ngay trong ô Layer 5: lấy từ WiFi/Ethernet stack và MQTT/server response |
| Cloud/Dashboard làm người đọc tưởng là task | Đổi thành `ThingsBoard/Server`; đặt ở ngoài biên ESP32 |
| Không biết `SensorData_t` vào Layer 5 lấy từ Layer 1 hay Layer 4 | Ghi rõ: Layer 5 lấy bản mới nhất theo `timestamp`; có thể đến từ Layer 1 hoặc Layer 4 |
| Queue/Semaphore khó hiểu | Queue dùng để truyền dữ liệu; Semaphore/Mutex chỉ dùng để báo hiệu hoặc bảo vệ shared latest buffer |

---

# 6. Bảng kết luận bản tối ưu

| Thành phần | Kết luận chốt |
|---|---|
| Kiến trúc dữ liệu | Cascading Model: Sensor -> GDD/CGDD Control -> Actuator -> Feedback -> IoT |
| Kiến trúc RTOS | Preemptive Scheduling + RMS Priority Assignment + Queue + Semaphore/Mutex nếu cần + Watchdog |
| Số layer nghiệp vụ | 5 layer |
| RTOS | Tầng nền, không phải layer nghiệp vụ |
| Cloud | Endpoint ngoài ESP32, không phải task nội bộ |
| Task_Cloud | Task của Layer 5 |
| Feedback | Layer 4 duy nhất |
| Bản nên dùng để vẽ lại | Bảng ở mục 2 và mục 3 của file này |

