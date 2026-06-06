# Command flow - Layer 2 Edge Processing & Control

Nguồn chốt: `flow task control.jpg`, `flow các layer.jpg`, `Tổng quan.pdf`, `design-rules-bien-va-chu-ky.md`

Layer 2 dùng các biến đã chốt:

```text
SensorData_t
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
ControlData_t
```

---

# Layer 2 - Edge Processing & Control Layer

```text
[Layer 2. Edge Processing & Control Layer]
Đối tượng: ESP32 + FreeRTOS + Task_Control
Công việc: Nhận SensorData_t, kiểm tra lỗi, cập nhật GDD/CGDD, xác định current_stage, chọn luật tưới theo Stage và tạo pump_cmd
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] GDD, CGDD, current_stage, soil_state, WATER_DURATION_MS, control_status
[BIẾN RA] pump_cmd, ControlData_t
Giải thích: Layer 2 quyết định tưới theo Stage. Layer 3 chỉ nhận pump_cmd và thực hiện bật/tắt bơm.
```

v

```text
[L2-1. Nhận SensorData_t]
Đối tượng: sensorToControlQueue
Công việc: Nhận dữ liệu cảm biến từ Layer 1
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t gồm dữ liệu DHT22 và Soil sensor: T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp.
```

v

```text
[L2-2. Kiểm tra dữ liệu cảm biến]
Đối tượng: Task_Control
Công việc: Kiểm tra DHT_status, Soil_status và Error_Flag
[BIẾN VÀO] DHT_status, Soil_status, Error_Flag
[BIẾN DÙNG] data_valid, control_status
[BIẾN RA] data_valid, control_status
Giải thích: Nếu Soil_status lỗi thì không tưới tự động. Nếu DHT_status lỗi thì không cập nhật GDD/CGDD và dùng CGDD gần nhất.
```

Nếu `Soil_status = ERROR`:

```text
[L2-E1. Lỗi Soil sensor]
Đối tượng: Task_Control
Công việc: Không tưới tự động
[BIẾN VÀO] Soil_status, Error_Flag
[BIẾN DÙNG] control_status
[BIẾN RA] pump_cmd = OFF, pump_state = OFF, WATER_DURATION_MS = 0, control_status = SOIL_ERROR
Giải thích: Soil sensor lỗi thì không đủ cơ sở quyết định độ ẩm đất, nên tắt bơm an toàn.
```

Nếu `DHT_status = ERROR`:

```text
[L2-E2. Lỗi DHT22]
Đối tượng: Task_Control
Công việc: Không cập nhật GDD/CGDD
[BIẾN VÀO] DHT_status, Error_Flag
[BIẾN DÙNG] CGDD, control_status
[BIẾN RA] CGDD, control_status
Giải thích: DHT22 lỗi thì dùng CGDD gần nhất, không tính thêm GDD mới trong chu kỳ này.
```

Nếu dữ liệu hợp lệ:

v

```text
[L2-3. Cập nhật GDD/CGDD]
Đối tượng: ESP32 + thuật toán GDD
Công việc: Tính GDD_increment và cập nhật CGDD
[BIẾN VÀO] T_air, T_base, timestamp
[BIẾN DÙNG] GDD_increment, GDD, CGDD
[BIẾN RA] GDD, CGDD
Giải thích: GDD_increment được tính từ T_air và T_base, sau đó cộng vào CGDD. Theo Tổng quan.pdf, GDD/CGDD dùng để xác định Stage cho ngày hôm sau.
```

v

```text
[L2-4. Xác định current_stage]
Đối tượng: Thuật toán GDD/CGDD
Công việc: Xác định giai đoạn cây từ CGDD
[BIẾN VÀO] CGDD
[BIẾN DÙNG] current_stage
[BIẾN RA] current_stage
Giải thích: current_stage có 3 giá trị: Stage 1 - cây non, Stage 2 - cây phát triển mạnh, Stage 3 - cây trưởng thành.
```

v

```text
[L2-5. Chọn soil_state theo Stage và H_soil]
Đối tượng: Thuật toán tưới chi tiết
Công việc: Dựa vào current_stage và H_soil để xác định trạng thái đất
[BIẾN VÀO] current_stage, H_soil
[BIẾN DÙNG] soil_state
[BIẾN RA] soil_state
Giải thích: Layer 2 dùng đúng bảng luật Stage 1/2/3 trong design-rules. Ví dụ Stage 1: H_soil > 70% là quá ẩm, 55-70% đủ ẩm, 40-55% hơi khô, 25-40% khô, <=25% rất khô.
```

v

```text
[L2-6. Chọn WATER_DURATION_MS theo soil_state]
Đối tượng: Thuật toán tưới chi tiết
Công việc: Chọn thời gian tưới cơ bản theo current_stage và soil_state
[BIẾN VÀO] current_stage, soil_state, H_soil
[BIẾN DÙNG] WATER_DURATION_MS
[BIẾN RA] WATER_DURATION_MS
Giải thích: Stage 1 chọn 0s, 5s, 8s, 15s theo H_soil. Stage 2 chọn 0s, 10s, 18s. Stage 3 chọn 0s, 10s, 20s.
```

v

```text
[L2-7. Hiệu chỉnh WATER_DURATION_MS theo T_air và H_air]
Đối tượng: Thuật toán tưới thích nghi
Công việc: Tăng hoặc giảm thời gian tưới theo nhiệt độ và độ ẩm không khí
[BIẾN VÀO] current_stage, T_air, H_air, H_soil, WATER_DURATION_MS
[BIẾN DÙNG] WATER_DURATION_MS
[BIẾN RA] WATER_DURATION_MS
Giải thích: Stage 1: T_air > 32°C cộng 3s, H_air < 50% cộng 5s nếu H_soil <= 25%, H_air > 90% trừ 2s. Stage 2: T_air > 32°C cộng 5s, H_air < 45% cộng 5s, H_air > 90% trừ 2s. Stage 3: T_air > 35°C cộng 5s, H_air < 45% cộng 3s, H_air > 90% trừ 2s.
```

v

```text
[L2-8. Kiểm tra thời gian nghỉ tưới]
Đối tượng: Task_Control
Công việc: Đảm bảo không tưới quá gần nhau
[BIẾN VÀO] last_watering_time, WATER_DURATION_MS
[BIẾN DÙNG] MIN_WATER_INTERVAL, control_status
[BIẾN RA] control_status
Giải thích: MIN_WATER_INTERVAL đã chốt là 30 phút = 1800000 ms. Nếu WATER_DURATION_MS > 0 nhưng chưa đủ 30 phút từ lần tưới gần nhất thì không tưới và đặt control_status = SAFETY_LOCK.
```

Nếu `WATER_DURATION_MS = 0`:

```text
[L2-E3. Không tưới do đất đủ ẩm hoặc quá ẩm]
Đối tượng: Task_Control
Công việc: Tắt bơm
[BIẾN VÀO] soil_state, WATER_DURATION_MS
[BIẾN DÙNG] relay, pump_state, control_status
[BIẾN RA] pump_cmd = OFF, pump_state = OFF, control_status = SOIL_MOISTURE_OK
Giải thích: Đất đủ ẩm hoặc quá ẩm thì không tưới. Nếu quá ẩm thì ghi trạng thái cảnh báo úng trong phần giải thích/log.
```

Nếu chưa đủ thời gian nghỉ:

```text
[L2-E4. Chưa đủ thời gian nghỉ]
Đối tượng: Task_Control
Công việc: Khóa tưới tạm thời
[BIẾN VÀO] last_watering_time
[BIẾN DÙNG] MIN_WATER_INTERVAL, control_status
[BIẾN RA] pump_cmd = OFF, pump_state = OFF, control_status = SAFETY_LOCK
Giải thích: Nếu chưa đủ 30 phút kể từ lần tưới gần nhất, hệ thống chờ thêm để tránh tưới liên tục quá gần nhau.
```

Nếu `WATER_DURATION_MS > 0` và đã đủ thời gian nghỉ:

v

```text
[L2-9. Bật bơm tưới]
Đối tượng: Task_Control
Công việc: Tạo lệnh bật bơm gửi sang Layer 3
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] relay, pump_state, control_status
[BIẾN RA] pump_cmd = ON, pump_state = ON, control_status = WATERING
Giải thích: Layer 2 tạo pump_cmd để Layer 3 bật bơm.
```

Sang Layer 3:

```text
pump_cmd
```

v

```text
[L2-10. Giữ bơm chạy trong WATER_DURATION_MS]
Đối tượng: Task_Control
Công việc: Giữ trạng thái tưới trong thời gian đã chọn
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] relay, pump_state
[BIẾN RA] watering_time
Giải thích: Bơm được giữ chạy theo WATER_DURATION_MS đã chọn từ bảng luật Stage.
```

v

```text
[L2-11. Tắt bơm sau khi tưới]
Đối tượng: Task_Control
Công việc: Tắt bơm và cập nhật thời điểm tưới
[BIẾN VÀO] watering_time
[BIẾN DÙNG] relay, pump_state, last_watering_time
[BIẾN RA] pump_cmd = OFF, pump_state = OFF, watering_time, control_status = WATERING_DONE
Giải thích: Kết thúc lần tưới, cập nhật thời gian tưới gần nhất để dùng cho chu kỳ sau.
```

v

```text
[L2-12. Đóng gói ControlData_t]
Đối tượng: Task_Control
Công việc: Tạo ControlData_t và gửi Task_Cloud
[BIẾN VÀO] H_soil, H_threshold, pump_state, control_status, watering_time, timestamp
[BIẾN DÙNG] ControlData_t
[BIẾN RA] ControlData_t
Giải thích: ControlData_t gồm kết quả điều khiển để Task_Cloud gửi lên dashboard/cloud.
```

---

# Mũi tên nối Layer 2

```text
Layer 1 [Đóng gói SensorData_t]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```

```text
Layer 2 [L2-9. Bật bơm tưới]
-> Layer 3 [Nhận pump_cmd]
Biến truyền: pump_cmd
```

```text
Layer 3 [Cập nhật pump_state]
-> Layer 2 [L2-12. Đóng gói ControlData_t]
Biến truyền: pump_state
```

```text
Layer 2 [L2-12. Đóng gói ControlData_t]
-> Layer 4 [Task_Cloud]
Biến truyền: ControlData_t
```
