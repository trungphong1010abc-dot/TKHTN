# Command flow - Đối tượng điều khiển Đất + cây trồng

Nguồn chốt: `flow các layer.jpg`, `flow layer actuator.jpg`, `các tầng khác chưa chọn flow.jpg`, `design-rules-bien-va-chu-ky.md`, `review-tong-quan-5-tang-command-flow.md`

File này mô tả phần vật lý nằm giữa `Actuator Layer` và `Feedback / Monitoring Layer`.

```text
Actuator Layer
-> Đất + cây trồng
-> Feedback / Monitoring Layer
-> quay lại Sensor Layer cho chu kỳ đo tiếp theo
```

`Đất + cây trồng` không phải một layer phần mềm riêng, không phải Task RTOS, không tạo queue và không tạo struct mới. Đây là đối tượng vật lý bị máy bơm tác động.

## Vai trò

```text
Đối tượng điều khiển:
Đất
cây trồng
vùng rễ
nước tưới
```

Biến/tác động nhận từ tầng chấp hành:

```text
pump_state
watering_time
```

Kết quả vật lý được đo lại ở tầng phản hồi:

```text
H_soil
Soil_status
timestamp
```

Ghi chú quan trọng:

```text
Không gọi phần này là Layer 6 trong sơ đồ 5 tầng.
Không thêm biến mới cho phần này.
H_soil thay đổi là kết quả vật lý, chỉ được hệ thống biết lại sau khi Soil Sensor đo.
```

---

## Đối tượng điều khiển - Đất + cây trồng

```text
[Đối tượng điều khiển. Đất + cây trồng]
Đối tượng: Đất + cây trồng + vùng rễ
Công việc: Nhận nước tưới từ máy bơm, làm độ ẩm đất vùng rễ thay đổi
[BIẾN VÀO] pump_state, watering_time
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil thay đổi ở chu kỳ đo tiếp theo
Giải thích: Đây là phần vật lý thật của hệ thống tưới. Khi Actuator Layer bật máy bơm, nước đi vào đất, thấm đến vùng rễ và làm H_soil thực tế thay đổi. Hệ thống chưa biết ngay giá trị mới này cho đến khi Feedback / Monitoring Layer đo lại.
```

v

```text
[OBJ-1. Đất nhận nước từ máy bơm]
Đối tượng: Đất tại khu vực đặt cây
Công việc: Nhận nước tưới khi máy bơm đang ON
[BIẾN VÀO] pump_state, watering_time
[BIẾN DÙNG] pump_state, watering_time
[BIẾN RA] H_soil có khả năng tăng ở chu kỳ đo tiếp theo
Giải thích: Nếu pump_state = ON thì máy bơm cấp nước vào đất. watering_time càng dài thì lượng nước đi vào đất càng nhiều. Nếu pump_state = OFF thì đất không nhận thêm nước từ máy bơm.
```

v

```text
[OBJ-2. Nước thấm vào vùng rễ]
Đối tượng: Đất + vùng rễ
Công việc: Nước phân bố và thấm dần trong đất quanh rễ cây
[BIẾN VÀO] pump_state, watering_time
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil thay đổi theo khả năng thấm và giữ nước của đất
Giải thích: Sau khi tưới, H_soil không nhất thiết thay đổi tức thời tại mọi vị trí. Đất cần thời gian để nước thấm xuống, phân bố quanh vùng rễ và ổn định lại trước khi Soil Sensor đo chu kỳ sau.
```

v

```text
[OBJ-3. Cây trồng hấp thụ nước]
Đối tượng: Cây trồng + bộ rễ
Công việc: Rễ cây hấp thụ nước từ vùng đất quanh rễ
[BIẾN VÀO] H_soil, current_stage
[BIẾN DÙNG] H_soil, current_stage
[BIẾN RA] H_soil có thể giảm dần theo thời gian
Giải thích: Cây sử dụng nước trong đất cho quá trình sinh trưởng. current_stage chỉ là thông tin về giai đoạn cây đã được Control Decision Layer xác định; đối tượng vật lý này không tính lại current_stage.
```

v

```text
[OBJ-4. Độ ẩm đất vùng rễ ổn định sau tưới]
Đối tượng: Đất vùng rễ
Công việc: Hình thành trạng thái độ ẩm đất mới sau khi nhận nước và sau khi cây hấp thụ nước
[BIẾN VÀO] pump_state, watering_time, H_soil
[BIẾN DÙNG] H_soil
[BIẾN RA] H_soil thực tế trong đất ở thời điểm đo sau
Giải thích: H_soil thực tế là kết quả của lượng nước được bơm vào, khả năng giữ nước của đất, lượng nước cây hấp thụ và quá trình mất nước tự nhiên. Đây là kết quả vật lý, không phải biến mới do phần này tự tạo.
```

v

```text
[OBJ-5. Chờ tầng phản hồi đo lại]
Đối tượng: Đất vùng rễ + Soil Sensor
Công việc: Trạng thái H_soil mới được Soil Sensor đo lại ở chu kỳ tiếp theo
[BIẾN VÀO] H_soil thực tế trong đất
[BIẾN DÙNG] Soil_status, timestamp
[BIẾN RA] H_soil, Soil_status, timestamp
Giải thích: Feedback / Monitoring Layer dùng Soil Sensor để đo lại H_soil, kiểm tra Soil_status và gắn timestamp. Dữ liệu này quay lại Sensor Layer / Signal Processing Layer để bắt đầu chu kỳ điều khiển tiếp theo.
```

---

## Mũi tên nối đối tượng điều khiển

```text
Actuator Layer [L3-4. Cập nhật trạng thái bơm]
-> Đối tượng điều khiển [OBJ-1. Đất nhận nước từ máy bơm]
Biến/tác động truyền: pump_state, watering_time
Ghi chú: pump_state cho biết bơm ON/OFF; watering_time cho biết thời gian bơm tác động lên đất.
```

```text
Control Decision Layer [L2-10. Giữ bơm chạy trong WATER_DURATION_MS]
-> Đối tượng điều khiển [OBJ-1. Đất nhận nước từ máy bơm]
Biến/tác động truyền: watering_time
Ghi chú: Đây là liên hệ logic từ quyết định tưới sang tác động vật lý. Tác động thực tế vẫn đi qua Actuator Layer.
```

```text
Đối tượng điều khiển [OBJ-1. Đất nhận nước từ máy bơm]
-> Đối tượng điều khiển [OBJ-2. Nước thấm vào vùng rễ]
Biến/tác động truyền: pump_state, watering_time
Ghi chú: Đây là mũi tên tác động vật lý, không tạo biến phần mềm mới.
```

```text
Đối tượng điều khiển [OBJ-2. Nước thấm vào vùng rễ]
-> Đối tượng điều khiển [OBJ-3. Cây trồng hấp thụ nước]
Biến/tác động truyền: H_soil, current_stage
Ghi chú: H_soil là trạng thái độ ẩm đất thực tế; current_stage mô tả nhu cầu nước theo giai đoạn cây.
```

```text
Đối tượng điều khiển [OBJ-3. Cây trồng hấp thụ nước]
-> Đối tượng điều khiển [OBJ-4. Độ ẩm đất vùng rễ ổn định sau tưới]
Biến/tác động truyền: H_soil
Ghi chú: H_soil có thể tăng, giữ nguyên hoặc giảm tùy lượng nước tưới, khả năng giữ nước và mức hấp thụ của cây.
```

```text
Đối tượng điều khiển [OBJ-4. Độ ẩm đất vùng rễ ổn định sau tưới]
-> Đối tượng điều khiển [OBJ-5. Chờ tầng phản hồi đo lại]
Biến/tác động truyền: H_soil
Ghi chú: Đây là bước chuyển từ kết quả vật lý sang dữ liệu có thể đo lại.
```

```text
Đối tượng điều khiển [OBJ-5. Chờ tầng phản hồi đo lại]
-> Feedback / Monitoring Layer [Soil Sensor đo lại H_soil]
Biến đo lại: H_soil, Soil_status, timestamp
Ghi chú: Feedback / Monitoring Layer quan sát kết quả sau khi bơm tác động lên đất/cây.
```

```text
Feedback / Monitoring Layer [Soil Sensor đo lại H_soil]
-> Sensor Layer [DHT22 + Soil Sensor + ADC]
Biến truyền cho chu kỳ sau: H_soil, Soil_status, timestamp
Ghi chú: Dữ liệu đo lại quay về đầu chu kỳ để tiếp tục đọc, xử lý tín hiệu và ra quyết định tưới tiếp theo.
```

```text
Sensor Layer / Signal Processing Layer [Đóng gói SensorData_t]
-> Control Decision Layer [Nhận SensorData_t]
Biến truyền: SensorData_t
Ghi chú: H_soil, Soil_status và timestamp sau khi đo lại được đóng gói vào SensorData_t để Control Decision Layer quyết định chu kỳ tưới tiếp theo.
```
