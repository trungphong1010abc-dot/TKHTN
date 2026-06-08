# FreeRTOS Scheduling - Câu hỏi, lựa chọn và đáp án đề xuất

Mục đích file này: biến các câu hỏi cấu hình FreeRTOS thành quyết định thiết kế dễ chọn, không yêu cầu phải biết sẵn code FreeRTOS.

---

# Cách hỏi theo góc nhìn thực tế / bán hàng

Nếu đang làm sản phẩm hoặc thuyết trình theo hướng thương mại, không nên hỏi người dùng cuối các câu như:

```text
Có bật preemption không?
Có bật time slicing không?
configTICK_RATE_HZ là bao nhiêu?
Queue dài bao nhiêu phần tử?
```

Các câu đó là câu hỏi kỹ thuật nội bộ. Với khách hàng hoặc người nghe không chuyên RTOS, nên đổi thành câu hỏi vận hành:

```text
1. Khi hệ thống quyết định tưới, bơm cần phản ứng nhanh đến mức nào?
2. Nếu đang gửi dữ liệu lên cloud, lệnh bơm có được phép bị chậm không? 
3. Nếu mất WiFi hoặc server lỗi, hệ thống có còn tự tưới tại chỗ không?
4. Dữ liệu cảm biến cần lấy bản mới nhất hay cần lưu nhiều bản lịch sử?
5. Telemetry lên cloud cần đúng từng mẫu đo hay chỉ cần đủ để giám sát?
6. Hệ thống ưu tiên an toàn tưới hay ưu tiên dashboard realtime?
7. Có cần tách rõ tác vụ điều khiển bơm khỏi tác vụ IoT để tăng độ ổn định không?
8. Nếu một task bị treo hoặc quá hạn một chu kỳ, hệ thống có cần tự phát hiện và chuyển về trạng thái an toàn không?
```

Đáp án sản phẩm đề xuất:

```text
1. Bơm phải phản ứng nhanh sau khi có quyết định tưới.
2. Lệnh bơm có thể bị chậm bởi việc gửi dữ liệu cloud, nhưng không vượt quá 1 chu kỳ T.
3. Nếu mất WiFi/server, hệ thống vẫn phải tự điều khiển tưới tại ESP32.
4. Với điều khiển, ưu tiên dữ liệu mới nhất theo timestamp.
5. Với cloud, telemetry có thể chậm hơn điều khiển nhưng không được làm sai trạng thái hệ thống.
6. Ưu tiên an toàn tưới và bảo vệ cây hơn dashboard realtime.
7. Cần tách task điều khiển, task actuator, task feedback và task cloud.
8. Cần dùng Watchdog Timer để phát hiện task treo/quá hạn và ưu tiên trạng thái an toàn.
```

Ghi chú:

```text
T là chu kỳ scheduling/chu kỳ phản ứng thiết kế dùng để giới hạn độ trễ tối đa của lệnh bơm.
Nếu chưa chốt T bằng số cụ thể, chỉ ghi T là giới hạn thiết kế và không tự đổi thành ms cố định.
```

Ánh xạ sang quyết định kỹ thuật:

```text
Bơm phải phản ứng nhanh
-> bật preemption
-> Task_Actuator priority cao nhất

Cloud được phép làm chậm lệnh bơm trong giới hạn tối đa 1 chu kỳ T
-> Task_Cloud vẫn có priority thấp hơn Task_Control và Task_Actuator
-> Nếu Task_Cloud đang chạy, Task_Control/Task_Actuator được ưu tiên để độ trễ không vượt quá T

DHT22 và Soil sensor quan trọng tương đương
-> có thể đặt cùng priority
-> bật time slicing để Round Robin giữa task cùng priority

Điều khiển cần dữ liệu mới nhất
-> queue length 1 hoặc shared latest buffer cho command/status
-> dùng timestamp để chọn SensorData_t mới nhất

Mất WiFi/server vẫn tưới được
-> Layer 2, Layer 3, Layer 4 chạy độc lập với Layer 5
-> cloud_status không được quyết định pump_cmd

Task bị treo/quá hạn
-> dùng Watchdog Timer
-> task quan trọng phải feed watchdog trong timeout cho phép
-> nếu Task_Control/Task_Actuator timeout, ưu tiên trạng thái an toàn, không phát lệnh tưới mới
```

Kết luận theo hướng sản phẩm:

```text
Người dùng/khách hàng không cần chọn preemption hoặc time slicing.
Nhóm thiết kế chọn cấu hình FreeRTOS dựa trên yêu cầu vận hành:
an toàn tưới, phản ứng nhanh, không phụ thuộc cloud, dữ liệu mới nhất.
```

Nếu đưa vào slide bán hàng hoặc thuyết trình dễ hiểu, nên nói:

```text
Hệ thống ưu tiên điều khiển bơm tại thiết bị trước, truyền dữ liệu cloud sau.
FreeRTOS giúp tách tác vụ để việc gửi telemetry không làm chậm quyết định tưới.
Khi có lệnh bơm, task actuator được ưu tiên chạy trước các tác vụ giám sát.
```

---

# Câu hỏi kỹ thuật nội bộ

Phần dưới đây dùng khi cần chuyển yêu cầu sản phẩm thành cấu hình FreeRTOS cụ thể.

---

## 0. Cascading Model có phải RTOS Scheduler không?

Câu hỏi:

```text
Cascading Model có phải là thuật toán lập lịch RTOS không?
```

Đáp án:

```text
Không.
```

Giải thích:

```text
Cascading Model là kiến trúc xử lý dữ liệu:
Task_Sensor -> Task_Control -> Task_Cloud.

Nó mô tả dữ liệu đi qua các bước:
Sensor -> GDD/CGDD -> Control -> ThingsBoard.

RTOS Scheduler là cơ chế chọn task nào chạy trên CPU.
Vì vậy phần kiến trúc phần mềm trình bày Cascading Model, còn phần RTOS trình bày Preemptive Scheduling + RMS + Queue + Semaphore + Watchdog.
```

---

## 1. Preemption chọn gì?

Câu hỏi:

```text
FreeRTOS có bật preemption không?
```

Vì sao phải hỏi:

```text
Preemption quyết định task priority cao có được ngắt task priority thấp ngay khi nó READY hay không.
Trong hệ tưới, Task_Actuator và Task_Control cần phản ứng nhanh hơn Task_Cloud.
Nếu không bật preemption, Task_Cloud hoặc task thấp hơn có thể giữ CPU lâu hơn mong muốn.
```

Lựa chọn:

```text
A. Bật preemption
B. Tắt preemption
```

Đáp án đề xuất:

```text
A. Bật preemption
```

Giải thích đáp án:

```text
Task_Actuator nhận pump_cmd cần chạy nhanh để bật/tắt bơm.
Task_Control cần xử lý SensorData_t và tạo pump_cmd kịp thời.
Task_Cloud chỉ gửi telemetry, không được làm chậm điều khiển bơm.
Vì vậy nên bật preemption để task điều khiển priority cao được chạy trước.
```

---

## 2. Time slicing chọn gì?

Câu hỏi:

```text
FreeRTOS có bật time slicing cho các task cùng priority không?
```

Vì sao phải hỏi:

```text
Time slicing quyết định các task cùng priority có được chia CPU theo Round Robin hay không.
Nếu Task_DHT22 và Task_Soil cùng priority, time slicing giúp hai task này không chờ nhau quá lâu khi cùng READY.
```

Lựa chọn:

```text
A. Bật time slicing
B. Tắt time slicing
```

Đáp án đề xuất:

```text
A. Bật time slicing
```

Giải thích đáp án:

```text
Task_DHT22 và Task_Soil đều là task cảm biến, mức quan trọng tương đương.
Nếu đặt cùng priority, bật time slicing giúp scheduler chia CPU công bằng theo tick.
Round Robin chỉ áp dụng trong nhóm cùng priority, không làm Task_Cloud ngang quyền với Task_Actuator.
```

---

## 3. Tick rate chọn gì?

Câu hỏi:

```text
configTICK_RATE_HZ nên chọn bao nhiêu?
```

Vì sao phải hỏi:

```text
Tick rate là nhịp đồng hồ của FreeRTOS.
Mỗi lần đồng hồ này "gõ" một nhịp, FreeRTOS mới kiểm tra lại task nào hết delay, task nào cần chạy tiếp, task nào cần đổi lượt CPU.

Ví dụ:
configTICK_RATE_HZ = 100 Hz
-> 1 tick = 10 ms
-> vTaskDelay(1 tick) nghĩa là chờ khoảng 10 ms
-> delay chỉ mịn theo bội số 10 ms

configTICK_RATE_HZ = 1000 Hz
-> 1 tick = 1 ms
-> vTaskDelay(1 tick) nghĩa là chờ khoảng 1 ms
-> delay mịn hơn, dễ biểu diễn các mốc ms nhỏ

Đổi lại:
Tick rate càng cao thì FreeRTOS phải ngắt CPU nhiều lần hơn mỗi giây để kiểm tra scheduler.
Vì vậy thời gian chính xác hơn, nhưng CPU tốn thêm một phần nhỏ cho scheduler.
```

Lựa chọn:

```text
A. 100 Hz
B. 1000 Hz
C. Chưa chốt, chỉ dùng pdMS_TO_TICKS()
```

Đáp án đề xuất:

```text
C. Chưa chốt, chỉ dùng pdMS_TO_TICKS()
```

Giải thích đáp án:

```text
Hiện flow là thiết kế, chưa có file cấu hình FreeRTOS thật.
Chốt 100 Hz hoặc 1000 Hz lúc này sẽ dễ sai nếu code mẫu dùng cấu hình khác.
Ở mức flow, chỉ cần ghi delay bằng ms và quy đổi bằng pdMS_TO_TICKS().
Khi sang code thật mới đọc configTICK_RATE_HZ để chốt số.
```

Hiểu nhanh:

```text
Trong flow, ta nói "delay 500 ms" hoặc "delay 1000 ms".
Khi viết code, pdMS_TO_TICKS(500) sẽ tự đổi 500 ms thành số tick đúng theo configTICK_RATE_HZ.

Nếu tick = 10 ms:
pdMS_TO_TICKS(500) -> khoảng 50 tick.

Nếu tick = 1 ms:
pdMS_TO_TICKS(500) -> khoảng 500 tick.

Vì vậy flow không cần tự chốt tick rate ngay.
Flow chỉ cần chốt thời gian theo ms và dùng pdMS_TO_TICKS() khi sang code.
```

---

## 4. Chu kỳ task cảm biến chọn gì?

Câu hỏi:

```text
Task_DHT22 và Task_Soil chạy cùng chu kỳ hay tách chu kỳ?
```

Vì sao phải hỏi:

```text
RMS gán priority dựa trên chu kỳ.
Nếu DHT22 và Soil cùng chu kỳ thì có thể cùng priority và dùng Round Robin.
Nếu Soil cần đo nhanh hơn DHT22 thì Soil có thể được priority cao hơn.
```

Lựa chọn:

```text
A. Task_DHT22 và Task_Soil cùng chu kỳ 60s
B. Task_DHT22 60s, Task_Soil nhanh hơn 10-30s
C. Chưa chốt, chỉ ghi chu kỳ đo cảm biến chung
```

Đáp án đề xuất:

```text
A. Task_DHT22 và Task_Soil cùng chu kỳ 60s
```

Giải thích đáp án:

```text
Design rules hiện đã chốt chu kỳ đo cảm biến / Task_Cloud là 60s.
Chưa có yêu cầu Soil sensor đo nhanh hơn trong chu kỳ bình thường.
Đặt DHT22 và Soil cùng 60s giúp flow đơn giản, dễ giải thích RMS và Round Robin.
Riêng phản hồi sau tưới đã có Task_Feedback xử lý, không cần bắt Task_Soil thường kỳ chạy nhanh hơn.
```

---

## 5. Task_Control chạy kiểu gì?

Câu hỏi:

```text
Task_Control chạy theo chu kỳ cố định hay chạy khi có SensorData_t mới?
```

Vì sao phải hỏi:

```text
Task_Control là task quyết định tưới.
Nếu chạy khi có SensorData_t mới, hệ thống phản ứng đúng theo dữ liệu mới.
Nếu chạy periodic nhưng chưa có dữ liệu mới, có thể xử lý lại dữ liệu cũ.
```

Lựa chọn:

```text
A. Event-driven: chạy khi sensorToControlQueue có SensorData_t
B. Periodic: chạy cố định mỗi 60s
C. Kết hợp: chờ queue nhưng có timeout
```

Đáp án đề xuất:

```text
A. Event-driven: chạy khi sensorToControlQueue có SensorData_t
```

Giải thích đáp án:

```text
Layer 2 bắt đầu từ việc nhận SensorData_t.
Task_Control không nên tự chạy nếu chưa có dữ liệu cảm biến mới.
Event-driven qua queue giúp luồng rõ: cảm biến đo -> gửi SensorData_t -> Task_Control xử lý -> tạo pump_cmd.
Nếu sau này cần chống treo queue, có thể đổi sang phương án C.
```

---

## 6. Deadline phản ứng Task_Actuator chọn gì?

Câu hỏi:

```text
Task_Actuator cần phản ứng với pump_cmd trong bao lâu?
```

Vì sao phải hỏi:

```text
Deadline của Task_Actuator quyết định priority của nó.
Task này liên quan trực tiếp bật/tắt bơm, nên không nên để telemetry hoặc xử lý chậm hơn chặn lại.
```

Lựa chọn:

```text
A. Dưới 100 ms
B. Dưới 500 ms
C. Chưa chốt, chỉ ghi nhanh hơn Task_Cloud và Task_Feedback
```

Đáp án đề xuất:

```text
A. Dưới 100 ms
```

Giải thích đáp án:

```text
Bật/tắt bơm là tác động chấp hành, nên nên phản ứng nhanh sau khi Task_Control đã quyết định.
Dưới 100 ms là mức hợp lý để giải thích task actuator có priority cao nhất.
Mốc này không ảnh hưởng đến WATER_DURATION_MS; WATER_DURATION_MS vẫn do Layer 2 tính.
```

---

## 7. Queue length chọn gì?

Câu hỏi:

```text
Mỗi queue nên chứa bao nhiêu phần tử?
```

Vì sao phải hỏi:

```text
Queue length ảnh hưởng cách xử lý khi dữ liệu đến nhanh hơn task nhận.
Với lệnh bơm và trạng thái bơm, bản mới nhất thường quan trọng hơn việc giữ nhiều bản cũ.
```

Lựa chọn:

```text
A. 1 phần tử, giữ gói mới nhất
B. 3-5 phần tử, cho phép đệm ngắn
C. Chưa chốt, chỉ ghi queue truyền struct
```

Đáp án đề xuất:

```text
A. 1 phần tử, giữ gói mới nhất
```

Giải thích đáp án:

```text
pump_cmd và pump_state là trạng thái điều khiển, không nên xử lý hàng dài lệnh cũ.
SensorData_t cũng ưu tiên bản mới nhất theo timestamp.
Queue length 1 giúp flow dễ hiểu: task đích nhận trạng thái mới nhất.
Nếu code thật cần log hoặc không mất mẫu đo, mới cân nhắc queue dài 3-5 phần tử.
```

---

## 8. ESP32 core affinity chọn gì?

Câu hỏi:

```text
Có ghim task vào core cụ thể của ESP32 không?
```

Vì sao phải hỏi:

```text
ESP32 có thể chạy FreeRTOS trên nhiều core tùy framework/cấu hình.
Ghim core ảnh hưởng task nào chạy ở core nào, nhưng nếu chưa có code thì chốt core sớm dễ sai.
```

Lựa chọn:

```text
A. Không ghim core, để FreeRTOS/ESP-IDF phân phối
B. Ghim task điều khiển vào một core, task cloud vào core còn lại
C. Chưa chốt
```

Đáp án đề xuất:

```text
A. Không ghim core, để FreeRTOS/ESP-IDF phân phối
```

Giải thích đáp án:

```text
Flow hiện tập trung vào quan hệ task, queue và priority, chưa có code pin task vào core.
Không ghim core giúp tránh tự thêm chi tiết cấu hình chưa có trong flow.
Nếu code thật cần tách WiFi/MQTT khỏi điều khiển, lúc đó mới cân nhắc phương án B.
```

---

## 9. Watchdog Timer chọn gì?

Câu hỏi:

```text
Có dùng Watchdog Timer để giám sát task bị treo hoặc quá hạn không?
```

Vì sao phải hỏi:

```text
Trong hệ tưới, lỗi nguy hiểm không chỉ là sai dữ liệu mà còn là task bị kẹt.
Nếu Task_Actuator hoặc Task_Control bị treo, bơm có thể không được điều khiển đúng thời điểm.
Watchdog giúp phát hiện task không chạy đúng chu kỳ và kích hoạt xử lý an toàn.
```

Lựa chọn:

```text
A. Dùng Watchdog Timer cho các task quan trọng
B. Chỉ dùng timeout queue/delay, không dùng watchdog riêng
C. Chưa chốt
```

Đáp án đề xuất:

```text
A. Dùng Watchdog Timer cho các task quan trọng
```

Giải thích đáp án:

```text
Task_Actuator, Task_Control và Task_Feedback liên quan trực tiếp đến điều khiển tại thiết bị.
Nếu các task này treo quá thời gian cho phép, hệ thống cần biết để chuyển về hướng an toàn.
Watchdog không tạo pump_cmd và không quyết định tưới.
Watchdog chỉ giám sát task có còn chạy đúng chu kỳ hay không.
```

Timeout đề xuất:

```text
Task_Actuator: không vượt quá 1 chu kỳ T.
Task_Control: không vượt quá 1 chu kỳ điều khiển.
Task_Feedback: delay ổn định đất + thời gian đọc ADC/retry + margin.
Task_DHT22 / Task_Soil: không vượt quá 1 chu kỳ đo cảm biến.
Task_Cloud: 1-2 chu kỳ telemetry, vì cloud được phép chậm hơn điều khiển.
```

---

## 10. Queue và Semaphore dùng thế nào?

Câu hỏi:

```text
Queue và Semaphore khác nhau thế nào trong hệ thống này?
```

Vì sao phải hỏi:

```text
Nếu dùng sai, dễ biến Semaphore thành nơi truyền dữ liệu hoặc dùng Queue để khóa tài nguyên.
Trong FreeRTOS, hai cơ chế này có vai trò khác nhau.
```

Đáp án đề xuất:

```text
Queue dùng để truyền dữ liệu giữa task.
Semaphore dùng để đồng bộ hoặc bảo vệ tài nguyên dùng chung.
```

Giải thích:

```text
Queue:
Task_Sensor -> Task_Control: SensorData_t.
Task_Control -> Task_Actuator: pump_cmd.
Task_Actuator -> Task_Feedback: pump_state.
Task_Control -> Task_Cloud: ControlData_t.
Task_Sensor/Task_Feedback -> Task_Cloud: SensorData_t mới nhất.

Semaphore:
Dùng để báo hiệu sự kiện hoặc bảo vệ shared latest buffer nếu nhiều task cùng truy cập.
Semaphore không thay thế Queue khi cần truyền struct.
Nếu code thật không có tài nguyên dùng chung, không bắt buộc dùng Semaphore.
```

---

# Bộ đáp án đề xuất để chốt nhanh

```text
0. Cascading Model không phải RTOS Scheduler
1A - Bật preemption
2A - Bật time slicing
3C - Chưa chốt tick rate, dùng pdMS_TO_TICKS()
4A - Task_DHT22 và Task_Soil cùng chu kỳ 60s
5A - Task_Control event-driven theo sensorToControlQueue
6A - Task_Actuator phản ứng dưới 100 ms
7A - Queue length 1, giữ gói mới nhất
8A - Không ghim core
9A - Dùng Watchdog Timer cho các task quan trọng
10. Queue truyền dữ liệu, Semaphore đồng bộ/bảo vệ tài nguyên
```

Diễn giải ngắn:

```text
Hệ thống dùng priority-based preemptive scheduling.
Task_Actuator cao nhất vì điều khiển bơm.
Task_Control cao vì quyết định tưới.
Task cảm biến và Task_Feedback ở mức trung bình.
Task_Cloud thấp vì telemetry không được làm chậm điều khiển.
Round Robin chỉ dùng cho task cùng priority như Task_DHT22 và Task_Soil.
```
