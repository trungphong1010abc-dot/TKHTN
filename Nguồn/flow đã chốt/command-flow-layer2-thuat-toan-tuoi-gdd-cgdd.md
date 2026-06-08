# Command flow - Layer 2 GDD/CGDD Control

Vai trò:

```text
Layer 2 nhận SensorData_t, kiểm tra dữ liệu, tính GDD/CGDD, xác định stage, chọn luật tưới, tạo pump_cmd và ControlData_t.
Layer 2 không xuất GPIO trực tiếp và không publish MQTT.
```

Đối tượng:

```text
Task_Control
sensorToControlQueue
actuatorCmdQueue
controlToCloudQueue
```

Biến đầu vào:

```text
SensorData_t
control_mode
manual_cmd
```

Biến đầu ra:

```text
pump_cmd
ControlData_t
```

Ghi chú về `control_mode` và `manual_cmd`:

```text
Hai biến này chỉ dùng nếu hệ thống có chức năng điều khiển thủ công từ ThingsBoard/Server.
Chúng là đầu vào tùy chọn của Task_Control, không thay thế luật an toàn tưới tự động.
Nếu chưa triển khai cloud command, vẽ nhánh này là "tùy chọn/chưa dùng".
```

---

# Flow chi tiết

```text
[Layer 2. GDD/CGDD Control Layer]
Đối tượng: Task_Control
Công việc: Xử lý dữ liệu cảm biến và tạo quyết định tưới
[BIẾN VÀO] SensorData_t, control_mode, manual_cmd
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, GDD, CGDD, current_stage, WATER_DURATION_MS
[BIẾN RA] pump_cmd, ControlData_t
Giải thích: Đây là tầng quyết định tưới của ESP32. Hệ thống vẫn tự điều khiển tại ESP32 nếu mất WiFi/server.
```

v

```text
[L2-1. Nhận SensorData_t]
Đối tượng: Task_Control + sensorToControlQueue
Công việc: Nhận dữ liệu cảm biến mới nhất
[BIẾN VÀO] SensorData_t
[BIẾN DÙNG] T_air, H_air, H_soil, DHT_status, Soil_status, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: SensorData_t có thể đến từ Layer 1 hoặc Layer 4; bản mới hơn được nhận theo timestamp.
```

v

```text
[L2-2. Kiểm tra dữ liệu cảm biến]
Đối tượng: Task_Control
Công việc: Kiểm tra trạng thái DHT22 và Soil sensor
[BIẾN VÀO] DHT_status, Soil_status, Error_Flag
[BIẾN DÙNG] DHT_status, Soil_status, Error_Flag, data_valid
[BIẾN RA] data_valid, control_status
Giải thích: Nếu dữ liệu lỗi, Task_Control phải chuyển sang hướng an toàn, không tưới dựa trên dữ liệu sai.
```

v

```text
[L2-3. Xử lý lỗi Soil sensor]
Đối tượng: Task_Control
Công việc: Chặn tưới tự động nếu Soil sensor lỗi
[BIẾN VÀO] Soil_status, Error_Flag
[BIẾN DÙNG] Soil_status, Error_Flag, pump_cmd, control_status
[BIẾN RA] pump_cmd, control_status
Giải thích: Nếu Soil_status lỗi thì pump_cmd = OFF và control_status = SOIL_ERROR.
```

Nếu Soil sensor lỗi:

```text
-> L2-16. Đóng gói ControlData_t
```

Nếu Soil sensor OK:

v

```text
[L2-4. Xử lý lỗi DHT22]
Đối tượng: Task_Control
Công việc: Quyết định có cập nhật GDD/CGDD mới hay không
[BIẾN VÀO] DHT_status, T_air
[BIẾN DÙNG] DHT_status, T_air, GDD, CGDD, current_stage
[BIẾN RA] current_stage
Giải thích: Nếu DHT22 lỗi, không cập nhật GDD/CGDD mới; dùng current_stage gần nhất.
```

v

```text
[L2-5. Cập nhật GDD/CGDD]
Đối tượng: Task_Control
Công việc: Cập nhật nhiệt độ ngày và cộng dồn GDD/CGDD khi chốt ngày
[BIẾN VÀO] T_air, timestamp
[BIẾN DÙNG] T_max, T_min, T_avg, T_base, GDD_daily, GDD, CGDD
[BIẾN RA] GDD, CGDD
Giải thích: CGDD chỉ cộng một lần khi chốt ngày, không cộng sau mỗi lần đọc cảm biến.
```

Công thức:

```text
T_avg = (T_max + T_min) / 2
GDD_daily = T_avg - T_base
Nếu GDD_daily < 0 thì GDD_daily = 0
CGDD = CGDD + GDD_daily
```

v

```text
[L2-6. Xác định current_stage]
Đối tượng: Task_Control
Công việc: Xác định giai đoạn cây theo CGDD
[BIẾN VÀO] CGDD
[BIẾN DÙNG] CGDD, current_stage
[BIẾN RA] current_stage
Giải thích: current_stage quyết định bộ ngưỡng H_soil và thời lượng tưới.
```

v

```text
[L2-7. Kiểm tra control_mode]
Đối tượng: Task_Control
Công việc: Chọn chế độ AUTO hoặc MANUAL nếu hệ thống có cloud command
[BIẾN VÀO] control_mode, manual_cmd
[BIẾN DÙNG] control_mode, manual_cmd
[BIẾN RA] control_mode
Giải thích: Nếu chưa triển khai cloud command, mặc định control_mode = AUTO.
```

Nếu `control_mode = MANUAL`:

```text
[L2-8. Xử lý Manual mode]
Đối tượng: Task_Control
Công việc: Tạo pump_cmd theo manual_cmd nhưng vẫn giữ các khóa an toàn cơ bản
[BIẾN VÀO] manual_cmd, Soil_status
[BIẾN DÙNG] manual_cmd, pump_cmd, control_status
[BIẾN RA] pump_cmd, control_status
Giải thích: manual_cmd = ON tạo pump_cmd = ON nếu không có lỗi an toàn; manual_cmd = OFF tạo pump_cmd = OFF.
```

Sau đó:

```text
-> L2-16. Đóng gói ControlData_t
```

Nếu `control_mode = AUTO`:

v

```text
[L2-9. Chọn luật tưới theo current_stage]
Đối tượng: Task_Control
Công việc: Chọn soil_state, H_threshold và WATER_DURATION_MS theo Stage 1/2/3
[BIẾN VÀO] current_stage, H_soil
[BIẾN DÙNG] current_stage, H_soil, soil_state, H_threshold, WATER_DURATION_MS
[BIẾN RA] soil_state, H_threshold, WATER_DURATION_MS
Giải thích: Stage khác nhau có ngưỡng đất khô và thời lượng tưới khác nhau.
```

Bảng luật tưới rút gọn:

| Stage | Điều kiện H_soil | soil_state | WATER_DURATION_MS |
|---|---|---|---|
| Stage 1 | H_soil > 70% | quá ẩm | 0 |
| Stage 1 | 55% <= H_soil <= 70% | đủ ẩm | 0 |
| Stage 1 | 40% <= H_soil < 55% | hơi khô | 5s |
| Stage 1 | 25% <= H_soil < 40% | khô | 8s |
| Stage 1 | H_soil <= 25% | rất khô | 15s |
| Stage 2 | H_soil > 75% | quá ẩm | 0 |
| Stage 2 | 45% <= H_soil <= 75% | đủ ẩm | 0 |
| Stage 2 | 30% <= H_soil < 45% | khô vừa | 10s |
| Stage 2 | H_soil < 30% | rất khô | 18s |
| Stage 3 | H_soil > 80% | quá ẩm | 0 |
| Stage 3 | 40% <= H_soil <= 80% | đủ ẩm | 0 |
| Stage 3 | 25% <= H_soil < 40% | khô | 10s |
| Stage 3 | H_soil < 25% | rất khô | 20s |

v

```text
[L2-10. Hiệu chỉnh WATER_DURATION_MS theo T_air và H_air]
Đối tượng: Task_Control
Công việc: Tăng/giảm thời lượng tưới theo nhiệt độ và độ ẩm không khí
[BIẾN VÀO] T_air, H_air, WATER_DURATION_MS
[BIẾN DÙNG] T_air, H_air, WATER_DURATION_MS
[BIẾN RA] WATER_DURATION_MS
Giải thích: Nếu WATER_DURATION_MS = 0 thì vẫn giữ không tưới.
```

v

```text
[L2-11. Kiểm tra có cần tưới không]
Đối tượng: Task_Control
Công việc: Kiểm tra WATER_DURATION_MS > 0
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] WATER_DURATION_MS, pump_cmd, control_status
[BIẾN RA] pump_cmd, control_status
Giải thích: Nếu WATER_DURATION_MS = 0 thì pump_cmd = OFF.
```

Nếu không cần tưới:

```text
-> L2-16. Đóng gói ControlData_t
```

Nếu cần tưới:

v

```text
[L2-12. Kiểm tra MIN_WATER_INTERVAL]
Đối tượng: Task_Control
Công việc: Bảo vệ cây bằng cách không tưới quá dày
[BIẾN VÀO] last_watering_time, MIN_WATER_INTERVAL, timestamp
[BIẾN DÙNG] last_watering_time, MIN_WATER_INTERVAL, pump_cmd, control_status
[BIẾN RA] pump_cmd, control_status
Giải thích: Chỉ cho phép tưới nếu now - last_watering_time >= MIN_WATER_INTERVAL.
```

Nếu chưa đủ thời gian nghỉ:

```text
pump_cmd = OFF
control_status = SAFETY_LOCK
-> L2-16. Đóng gói ControlData_t
```

Nếu đủ thời gian nghỉ:

v

```text
[L2-13. Tạo pump_cmd = ON]
Đối tượng: Task_Control
Công việc: Phát lệnh bật bơm
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] pump_cmd, control_status
[BIẾN RA] pump_cmd = ON
Giải thích: pump_cmd được gửi sang Task_Actuator qua actuatorCmdQueue.
```

v

```text
[L2-14. Chờ WATER_DURATION_MS]
Đối tượng: Task_Control
Công việc: Giữ trạng thái tưới trong thời lượng đã tính
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] WATER_DURATION_MS
[BIẾN RA] watering_duration
Giải thích: Đây là thời gian tưới do thuật toán quyết định.
```

v

```text
[L2-15. Tạo pump_cmd = OFF]
Đối tượng: Task_Control
Công việc: Phát lệnh tắt bơm sau khi đủ thời lượng tưới
[BIẾN VÀO] WATER_DURATION_MS
[BIẾN DÙNG] pump_cmd, last_watering_time, control_status
[BIẾN RA] pump_cmd = OFF
Giải thích: Sau khi tắt bơm, cập nhật last_watering_time và control_status = WATERING_DONE.
```

v

```text
[L2-16. Đóng gói ControlData_t]
Đối tượng: Task_Control
Công việc: Đóng gói trạng thái điều khiển để gửi Layer 5
[BIẾN VÀO] H_soil, H_threshold, current_stage, soil_state, pump_cmd, control_status, watering_duration, timestamp
[BIẾN DÙNG] ControlData_t
[BIẾN RA] ControlData_t
Giải thích: ControlData_t mô tả kết quả quyết định tưới và trạng thái điều khiển.
```

v

```text
[L2-17. Gửi ControlData_t sang Layer 5]
Đối tượng: Task_Control + controlToCloudQueue
Công việc: Gửi dữ liệu điều khiển cho Task_Cloud
[BIẾN VÀO] ControlData_t
[BIẾN DÙNG] ControlData_t, controlToCloudQueue
[BIẾN RA] ControlData_t
Giải thích: Layer 5 dùng ControlData_t để đóng gói telemetry.
```

---

# Mũi tên nối layer

```text
Layer 1 hoặc Layer 4
-> RTOS [sensorToControlQueue]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```

```text
Layer 2 [L2-13 hoặc L2-15. Tạo pump_cmd]
-> RTOS [actuatorCmdQueue]
-> Layer 3 [L3-1. Nhận pump_cmd]
Biến truyền: pump_cmd
```

```text
Layer 2 [L2-17. Gửi ControlData_t sang Layer 5]
-> RTOS [controlToCloudQueue]
-> Layer 5 [L5-2. Task_Cloud nhận ControlData_t]
Biến truyền: ControlData_t
```

