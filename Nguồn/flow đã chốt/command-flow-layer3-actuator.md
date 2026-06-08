# Command flow - Layer 3 Actuator

Vai trò:

```text
Layer 3 nhận pump_cmd từ Layer 2 và biến lệnh logic thành hành động bật/tắt máy bơm.
Layer 3 không tính GDD/CGDD, không quyết định tưới, không tạo ControlData_t và không publish MQTT.
```

Đối tượng:

```text
Task_Actuator
GPIO ESP32
AO3400 hoặc relay
máy bơm
actuatorCmdQueue
actuatorFeedbackQueue
```

Biến:

```text
Đầu vào: pump_cmd
Đầu ra: pump_state
```

---

# Flow chi tiết

```text
[Layer 3. Actuator Layer]
Đối tượng: Task_Actuator + GPIO ESP32 + AO3400/relay + máy bơm
Công việc: Nhận pump_cmd và điều khiển phần cứng bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: Đây là tầng chấp hành. ESP32 chỉ xuất tín hiệu điều khiển; AO3400/relay mới đóng/ngắt nguồn bơm.
```

v

```text
[L3-1. Nhận pump_cmd]
Đối tượng: Task_Actuator + actuatorCmdQueue
Công việc: Nhận lệnh bật/tắt bơm từ Layer 2
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd, actuatorCmdQueue
[BIẾN RA] pump_cmd
Giải thích: pump_cmd = ON thì chuẩn bị bật bơm; pump_cmd = OFF thì chuẩn bị tắt bơm.
```

v

```text
[L3-2. ESP32 xuất tín hiệu GPIO]
Đối tượng: GPIO ESP32
Công việc: Xuất HIGH/LOW theo pump_cmd
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] tín hiệu GPIO
Giải thích: Không tạo biến nghiệp vụ mới cho GPIO; tín hiệu GPIO chỉ là kết quả phần cứng của pump_cmd.
```

v

```text
[L3-3. AO3400/relay đóng ngắt nguồn]
Đối tượng: AO3400 hoặc relay
Công việc: Đóng/ngắt dòng cấp cho máy bơm
[BIẾN VÀO] tín hiệu GPIO, pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] trạng thái cấp nguồn bơm
Giải thích: AO3400/relay là tầng công suất, không phải thuật toán điều khiển.
```

v

```text
[L3-4. Cập nhật pump_state]
Đối tượng: Task_Actuator + máy bơm
Công việc: Ghi nhận trạng thái bơm sau khi chấp hành
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: pump_state phản ánh trạng thái bơm ON/OFF sau khi actuator thực hiện lệnh.
```

v

```text
[L3-5. Gửi pump_state sang Layer 4]
Đối tượng: Task_Actuator + actuatorFeedbackQueue
Công việc: Gửi trạng thái bơm cho Task_Feedback
[BIẾN VÀO] pump_state
[BIẾN DÙNG] pump_state, actuatorFeedbackQueue
[BIẾN RA] pump_state
Giải thích: Layer 4 cần pump_state để biết khi nào đọc phản hồi độ ẩm đất sau tác động tưới.
```

---

# Mũi tên nối layer

```text
Layer 2 [L2-13 hoặc L2-15. Tạo pump_cmd]
-> RTOS [actuatorCmdQueue]
-> Layer 3 [L3-1. Nhận pump_cmd]
Biến truyền: pump_cmd
```

```text
Layer 3 [L3-5. Gửi pump_state sang Layer 4]
-> RTOS [actuatorFeedbackQueue]
-> Layer 4 [L4-1. Nhận pump_state và chờ đất ổn định]
Biến truyền: pump_state
```

