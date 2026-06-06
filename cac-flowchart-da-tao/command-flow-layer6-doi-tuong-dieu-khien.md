# Command flow - Layer 6 Đối tượng điều khiển

Nguồn bổ sung: tầng đối tượng điều khiển trong mô hình tưới.

Layer 6 là phần vật lý được hệ thống điều khiển trực tiếp:

```text
Đất
cây trồng
vùng rễ
nước tưới
```

Layer 6 không tạo biến điều khiển mới. Kết quả vật lý của tầng này được Layer 1 đo lại thành:

```text
H_soil
Soil_status
timestamp
```

---

## Layer 6 - Đối tượng điều khiển

```text
[Layer 6. Đối tượng điều khiển]
Đối tượng: Đất + cây trồng
Công việc: Nhận nước tưới từ máy bơm, làm độ ẩm đất vùng rễ thay đổi
[BIẾN VÀO] pump_state, watering_time
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil thay đổi ở chu kỳ đo tiếp theo
Giải thích: Đây là đối tượng vật lý thật của hệ thống. Khi Layer 3 bật máy bơm, nước đi vào đất, thấm đến vùng rễ và làm độ ẩm đất thay đổi. Sự thay đổi này không được tính trực tiếp bằng biến mới, mà được Soil Sensor đo lại ở Layer 1 trong chu kỳ sau.
```

v

```text
[L6-1. Nhận nước tưới]
Đối tượng: Đất tại khu vực đặt cây
Công việc: Nhận lượng nước được bơm ra trong thời gian tưới
[BIẾN VÀO] pump_state, watering_time
[BIẾN DÙNG] pump_state, watering_time
[BIẾN RA] nước đi vào đất
Giải thích: Nếu pump_state = ON thì máy bơm cấp nước vào đất. Thời gian bơm càng dài thì lượng nước đi vào đất càng nhiều.
```

v

```text
[L6-2. Nước thấm vào vùng rễ]
Đối tượng: Đất + vùng rễ
Công việc: Nước phân bố và thấm dần trong đất
[BIẾN VÀO] nước đi vào đất
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil tăng hoặc ổn định tùy khả năng thấm và giữ nước của đất
Giải thích: Sau khi tưới, nước không làm H_soil thay đổi tức thời tại mọi vị trí. Đất cần thời gian để thấm, phân bố nước và ổn định độ ẩm quanh vùng rễ.
```

v

```text
[L6-3. Cây trồng hấp thụ nước]
Đối tượng: Cây trồng + bộ rễ
Công việc: Rễ cây hấp thụ nước theo nhu cầu sinh trưởng
[BIẾN VÀO] H_soil, current_stage
[BIẾN DÙNG] H_soil, current_stage
[BIẾN RA] H_soil giảm dần theo thời gian
Giải thích: Cây hấp thụ nước từ vùng rễ. Nhu cầu nước phụ thuộc vào giai đoạn sinh trưởng current_stage và điều kiện môi trường.
```

v

```text
[L6-4. Độ ẩm đất thay đổi sau tưới]
Đối tượng: Đất vùng rễ
Công việc: Hình thành trạng thái độ ẩm mới của đất
[BIẾN VÀO] nước tưới, lượng nước cây hấp thụ
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil thực tế trong đất
Giải thích: H_soil thực tế là kết quả của nước được tưới vào, khả năng giữ nước của đất, lượng nước cây hấp thụ và quá trình mất nước tự nhiên.
```

v

```text
[L6-5. Chuẩn bị cho chu kỳ đo tiếp theo]
Đối tượng: Soil Sensor tại vùng rễ
Công việc: Soil Sensor đo lại độ ẩm đất sau khi đối tượng vật lý thay đổi
[BIẾN VÀO] H_soil thực tế trong đất
[BIẾN DÙNG] Soil_status, timestamp
[BIẾN RA] H_soil, Soil_status, timestamp
Giải thích: Ở chu kỳ tiếp theo, Layer 1 đọc Soil Sensor để lấy H_soil mới. Dữ liệu này quay lại Layer 2 để quyết định có tưới tiếp, dừng tưới hoặc khóa tưới an toàn.
```

---

## Mũi tên nối Layer 6

```text
Layer 3 [L3-5. Cập nhật trạng thái bơm]
-> Layer 6 [L6-1. Nhận nước tưới]
Biến truyền: pump_state, watering_time
```

```text
Layer 6 [L6-5. Chuẩn bị cho chu kỳ đo tiếp theo]
-> Layer 1 [L1-3. Đọc và xử lý cảm biến độ ẩm đất]
Biến đo lại: H_soil, Soil_status, timestamp
```

```text
Layer 1 [L1-5. Đóng gói dữ liệu cảm biến]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```
