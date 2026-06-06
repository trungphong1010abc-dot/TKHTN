# Command flow - Layer 3 Actuator

Nguồn chốt: `flow layer actuator.jpg`, `flow các layer.jpg`

Layer 3 chỉ dùng biến đã có trong ảnh:

```text
pump_cmd
pump_state
```

Thành phần phần cứng:

```text
GPIO ESP32
AO3400/relay
máy bơm
```

---

# Layer 3 - Tầng chấp hành

```text
[Layer 3. Tầng chấp hành]
Đối tượng: GPIO ESP32 + AO3400/relay + máy bơm
Công việc: Nhận pump_cmd từ Task_Control và bật/tắt máy bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: Layer này làm việc theo mô hình Digital Output -> Power Driver -> Actuator. ESP32 xuất tín hiệu HIGH/LOW, AO3400/relay đóng/ngắt dòng cấp cho bơm, máy bơm tạo hành động tưới nước.
```

v

```text
[L3-1. Nhận pump_cmd]
Đối tượng: Task_Actuator / GPIO ESP32
Công việc: Nhận lệnh điều khiển bơm từ Layer 2
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_cmd
Giải thích: pump_cmd = ON thì chuẩn bị bật bơm. pump_cmd = OFF thì giữ bơm ở trạng thái tắt.
```

v

```text
[L3-2. ESP32 xuất tín hiệu HIGH/LOW]
Đối tượng: GPIO ESP32
Công việc: Xuất tín hiệu điều khiển tầng công suất AO3400/relay
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_cmd
Giải thích: ESP32 không cấp nguồn trực tiếp cho bơm. ESP32 chỉ xuất tín hiệu điều khiển theo pump_cmd sang AO3400/relay.
```

v

```text
[L3-3. AO3400/relay đóng ngắt nguồn]
Đối tượng: AO3400/relay
Công việc: Đóng hoặc ngắt dòng cấp cho máy bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: Khi pump_cmd = ON, relay đóng nguồn và máy bơm ON. Khi pump_cmd = OFF, relay ngắt nguồn và máy bơm OFF.
```

v

```text
[L3-4. Cập nhật trạng thái bơm]
Đối tượng: Máy bơm
Công việc: Ghi nhận trạng thái ON/OFF của bơm
[BIẾN VÀO] pump_cmd
[BIẾN DÙNG] pump_cmd
[BIẾN RA] pump_state
Giải thích: pump_state phản ánh kết quả chấp hành sau khi AO3400/relay điều khiển máy bơm.
```

---

# Mũi tên nối Layer 3

```text
Layer 2 [Ô bật bơm tưới / tạo pump_cmd]
-> Layer 3 [L3-1. Nhận pump_cmd]
Biến truyền: pump_cmd
```

```text
Layer 3 [L3-4. Cập nhật trạng thái bơm]
-> Layer 2 [Ô đóng gói ControlData_t]
Biến truyền: pump_state
```

```text
Layer 2 [Ô đóng gói ControlData_t]
-> Layer 4 [Task_Cloud]
Biến truyền: ControlData_t
```
