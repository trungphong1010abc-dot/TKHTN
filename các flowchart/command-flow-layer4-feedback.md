# Command flow - Layer 4 Feedback Layer

Nguồn chốt: `các tầng khác chưa chọn flow.jpg`, `flow cảm biến điện dung.jpg`, `flow các layer.jpg`, `design-rules-bien-va-chu-ky.md`

Biến tạo ra để gửi sang layer khác:

```text
SensorData_t
```

Biến cập nhật nội bộ trong layer:

```text
ADC_filtered
H_soil
Soil_status
Soil_Error_Flag
Error_Flag
timestamp
```

Đối tượng phần cứng:

```text
Soil sensor
ADC ESP32
ESP32
```

Nguyên tắc của Layer 4:

```text
Layer 3
-> đất + cây nhận nước
-> Soil sensor đo lại H_soil
-> ESP32 đóng gói SensorData_t
-> Layer 2 xử lý ở chu kỳ sau
```

Layer 4 là tầng phản hồi của hệ điều khiển kín. Sau khi Layer 3 bật/tắt máy bơm, đất và cây nhận nước, độ ẩm đất thay đổi. Soil sensor đo lại `H_soil`, ESP32 cập nhật trạng thái Soil sensor và gửi dữ liệu phản hồi về Layer 2 cho chu kỳ quyết định tưới tiếp theo.

Layer 4 không quyết định tưới, không tạo `pump_cmd`, không điều khiển bơm và không publish cloud.

---

# Layer 4 - Feedback Layer

```text
[Layer 4. Feedback Layer]
Đối tượng: Soil sensor + ADC ESP32 + ESP32
Công việc: Đo lại H_soil sau tác động tưới và gửi dữ liệu phản hồi về controller cho chu kỳ sau
[BIẾN VÀO] pump_state, timestamp
[BIẾN DÙNG] ADC_filtered, H_soil, Soil_status, Soil_Error_Flag, Error_Flag, timestamp
[BIẾN RA] SensorData_t
Giải thích: Layer 4 phản ánh tác động thật của bơm lên đất. Khi bơm đã chạy hoặc dừng, Soil sensor đo lại độ ẩm đất để Layer 2 có dữ liệu mới cho lần quyết định tiếp theo.
```

v

```text
[L4-1. Chờ đất ổn định sau tưới]
Đối tượng: Đất + vùng rễ cây
Công việc: Chờ nước thấm vào vùng đất quanh cảm biến trước khi đọc lại Soil sensor
[BIẾN VÀO] pump_state
[BIẾN DÙNG] pump_state
[BIẾN RA] pump_state
Giải thích: Nếu vừa tưới xong, độ ẩm quanh cảm biến cần một khoảng ngắn để ổn định. Layer 4 không tính lượng nước, chỉ mô tả điểm phản hồi vật lý trước khi đo lại.
```

v

```text
[L4-2. Đọc lại ADC Soil sensor]
Đối tượng: Soil sensor + ADC ESP32
Công việc: Đọc lại tín hiệu độ ẩm đất từ cảm biến điện dung
[BIẾN VÀO] Không có
[BIẾN DÙNG] ADC_filtered
[BIẾN RA] ADC_filtered
Giải thích: ADC ESP32 đọc tín hiệu từ Soil sensor và lọc giá trị để giảm nhiễu trước khi đổi sang phần trăm độ ẩm đất.
```

v

```text
[L4-3. Tính lại H_soil]
Đối tượng: ESP32
Công việc: Chuyển ADC_filtered thành độ ẩm đất H_soil
[BIẾN VÀO] ADC_filtered
[BIẾN DÙNG] ADC_filtered, H_soil
[BIẾN RA] H_soil
Giải thích: H_soil là giá trị phản hồi chính của vòng kín. Layer 2 dùng H_soil mới để biết đất còn khô, đủ ẩm hay quá ẩm ở chu kỳ sau.
```

v

```text
[L4-4. Kiểm tra trạng thái Soil sensor]
Đối tượng: ESP32
Công việc: Kiểm tra giá trị đọc Soil sensor có hợp lệ hay không
[BIẾN VÀO] ADC_filtered, H_soil
[BIẾN DÙNG] Soil_status, Soil_Error_Flag, Error_Flag
[BIẾN RA] Soil_status, Soil_Error_Flag, Error_Flag
Giải thích: Nếu Soil sensor lỗi, Layer 4 cập nhật Soil_status và Error_Flag để Layer 2 không ra quyết định tưới tự động dựa trên dữ liệu sai.
```

v

```text
[L4-5. Cập nhật timestamp phản hồi]
Đối tượng: ESP32
Công việc: Gắn thời điểm đo phản hồi đất
[BIẾN VÀO] timestamp
[BIẾN DÙNG] timestamp
[BIẾN RA] timestamp
Giải thích: timestamp cho biết thời điểm dữ liệu phản hồi được đo, giúp Layer 2 và Layer 5 biết dữ liệu thuộc chu kỳ nào.
```

v

```text
[L4-6. Đóng gói SensorData_t phản hồi]
Đối tượng: ESP32
Công việc: Đóng gói dữ liệu phản hồi để gửi về Layer 2
[BIẾN VÀO] H_soil, Soil_status, Error_Flag, timestamp
[BIẾN DÙNG] SensorData_t
[BIẾN RA] SensorData_t
Giải thích: SensorData_t phản hồi mang H_soil mới sau tác động tưới. Layer 2 dùng dữ liệu này trong chu kỳ điều khiển tiếp theo.
```

---

# Mũi tên nối Layer 4

```text
Layer 3 [L3-4. Cập nhật pump_state]
-> Layer 4 [L4-1. Chờ đất ổn định sau tưới]
Biến truyền: pump_state
```

```text
Layer 4 [L4-6. Đóng gói SensorData_t phản hồi]
-> Layer 2 [L2-1. Nhận SensorData_t]
Biến truyền: SensorData_t
```

```text
Layer 4 [L4-6. Đóng gói SensorData_t phản hồi]
-> Layer 5 [Task_Cloud]
Biến truyền: SensorData_t
```
