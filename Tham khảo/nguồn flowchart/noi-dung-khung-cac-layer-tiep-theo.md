# Nội dung các khung layer tiếp theo

## Layer 3: Tầng chấp hành

### Khung tổng quan

Layer 3. Tầng chấp hành  
Gồm: ESP32 + relay + máy bơm  
Công việc: Nhận lệnh `pump_cmd` từ Layer 2 và điều khiển máy bơm  
Input: `pump_cmd`, `watering_duration`, `control_status`  
Output: `pump_state`, `watering_time`, `actuator_error`

### 1. Nhận lệnh điều khiển bơm

Đối tượng: ESP32 + relay  
Công việc: Nhận `pump_cmd` từ `ControlData_t`  
Biến vào: `pump_cmd`, `watering_duration`, `control_status`  
Giải thích: Layer 3 kiểm tra lệnh điều khiển do Layer 2 gửi xuống để quyết định bật hoặc tắt máy bơm.

### 2. Kiểm tra điều kiện an toàn trước khi bơm

Đối tượng: Task_Control / Actuator_Control  
Công việc: Kiểm tra trạng thái hệ thống trước khi kích hoạt relay  
Biến kiểm tra: `Error_Flag`, `pump_cmd`, `MAX_WATERING_TIME`  
Giải thích: Nếu cảm biến lỗi, dữ liệu không hợp lệ hoặc thời gian tưới vượt giới hạn thì không bật bơm.

### 3. Điều khiển relay

Đối tượng: GPIO ESP32 + relay  
Công việc: Xuất tín hiệu ON/OFF ra chân điều khiển relay  
Biến dùng: `pump_cmd`, `RELAY_ON`, `RELAY_OFF`  
Giải thích: Nếu `pump_cmd = ON` thì bật relay để cấp nguồn cho máy bơm. Nếu `pump_cmd = OFF` thì tắt relay.

### 4. Theo dõi thời gian tưới

Đối tượng: Timer / tick counter  
Công việc: Đếm thời gian máy bơm đang hoạt động  
Biến dùng: `watering_start_time`, `watering_duration`, `watering_time`  
Giải thích: Khi thời gian tưới đạt giá trị đặt trước, hệ thống tự động tắt bơm để tránh tưới quá lâu.

### 5. Cập nhật trạng thái chấp hành

Đối tượng: Actuator_Status  
Công việc: Cập nhật trạng thái thực tế của bơm  
Biến ra: `pump_state`, `watering_time`, `actuator_error`  
Giải thích: Sau mỗi lần bật/tắt bơm, hệ thống ghi lại trạng thái để gửi ngược về Layer 2 và Layer 4.

### 6. Gửi phản hồi về Layer 2

Đối tượng: ControlData_t / Queue phản hồi  
Công việc: Gửi trạng thái bơm về Task_Control  
Biến gửi: `pump_state`, `watering_time`, `actuator_error`, `timestamp`  
Giải thích: Layer 2 dùng dữ liệu phản hồi này để đóng gói log điều khiển và gửi lên cloud.

## Layer 4: Tầng truyền thông

### Khung tổng quan

Layer 4. Communication Layer  
Gồm: ESP32 WiFi + MQTT/HTTP  
Công việc: Gửi dữ liệu cảm biến và trạng thái điều khiển lên cloud  
Input: `SensorData_t`, `ControlData_t`, `pump_state`  
Output: `TelemetryPacket_t`, `cloud_status`, `remote_config`

### 1. Nhận dữ liệu từ các layer dưới

Đối tượng: Task_Cloud  
Công việc: Nhận dữ liệu từ Queue hoặc biến dùng chung  
Biến vào: `SensorData_t`, `ControlData_t`, `pump_state`  
Giải thích: Task_Cloud lấy dữ liệu cảm biến, quyết định điều khiển và trạng thái bơm để chuẩn bị gửi lên cloud.

### 2. Đóng gói dữ liệu truyền thông

Đối tượng: TelemetryPacket_t  
Công việc: Gom dữ liệu thành một gói thống nhất  
Biến dùng: `device_id`, `timestamp`, `T_air`, `H_air`, `H_soil`, `pump_state`, `Error_Flag`  
Giải thích: Gói dữ liệu giúp cloud nhận được đầy đủ trạng thái của hệ thống tại một thời điểm.

### 3. Kiểm tra kết nối WiFi

Đối tượng: ESP32 WiFi  
Công việc: Kiểm tra trạng thái kết nối mạng  
Biến dùng: `wifi_status`, `retry_count`, `cloud_status`  
Giải thích: Nếu mất WiFi thì dữ liệu được lưu tạm. Nếu WiFi ổn định thì tiếp tục gửi lên cloud.

### 4. Gửi dữ liệu lên cloud

Đối tượng: MQTT client hoặc HTTP client  
Công việc: Publish hoặc POST dữ liệu  
Biến gửi: `TelemetryPacket_t`  
Giải thích: ESP32 truyền dữ liệu cảm biến, trạng thái bơm và lỗi hệ thống lên server/cloud để lưu trữ và hiển thị.

### 5. Nhận cấu hình từ cloud

Đối tượng: MQTT subscribe hoặc HTTP polling  
Công việc: Nhận cấu hình mới từ người dùng  
Biến nhận: `remote_config`, `H_soil_threshold`, `watering_schedule`, `manual_cmd`, `auto_mode`  
Giải thích: Khi người dùng thay đổi ngưỡng hoặc bật/tắt bơm từ xa, cloud gửi cấu hình mới về ESP32.

### 6. Gửi cấu hình về Layer 2

Đối tượng: ConfigData_t  
Công việc: Chuyển cấu hình cloud cho Task_Control  
Biến gửi: `H_soil_threshold`, `watering_schedule`, `manual_cmd`, `auto_mode`  
Giải thích: Layer 2 dùng cấu hình mới để thay đổi thuật toán quyết định tưới.

## Layer 5: Tầng cloud và lưu trữ

### Khung tổng quan

Layer 5. Cloud Storage & Service Layer  
Gồm: Database + API/MQTT Broker  
Công việc: Lưu dữ liệu, xử lý trạng thái và quản lý cấu hình thiết bị  
Input: `TelemetryPacket_t`  
Output: `remote_config`, `alert_event`, dữ liệu hiển thị dashboard

### 1. Nhận dữ liệu từ ESP32

Đối tượng: MQTT Broker / API Server  
Công việc: Nhận gói `TelemetryPacket_t` từ Layer 4  
Biến nhận: `device_id`, `timestamp`, `SensorData_t`, `ControlData_t`, `pump_state`  
Giải thích: Cloud tiếp nhận dữ liệu định kỳ từ thiết bị để lưu trữ và xử lý.

### 2. Lưu dữ liệu cảm biến

Đối tượng: Database  
Công việc: Ghi dữ liệu vào bảng log  
Biến lưu: `T_air`, `H_air`, `H_soil`, `DHT_status`, `Soil_status`, `timestamp`  
Giải thích: Dữ liệu cảm biến được lưu theo thời gian để phục vụ biểu đồ và phân tích lịch sử.

### 3. Lưu dữ liệu điều khiển

Đối tượng: Database  
Công việc: Ghi lại quyết định tưới và trạng thái bơm  
Biến lưu: `pump_cmd`, `pump_state`, `watering_time`, `control_status`  
Giải thích: Log điều khiển giúp kiểm tra hệ thống đã tưới khi nào, trong bao lâu và vì lý do gì.

### 4. Xử lý cảnh báo

Đối tượng: Cloud service  
Công việc: Kiểm tra điều kiện bất thường  
Biến kiểm tra: `H_soil`, `Error_Flag`, `actuator_error`, `watering_time`  
Giải thích: Nếu đất quá khô, cảm biến lỗi hoặc bơm chạy quá lâu thì tạo cảnh báo cho người dùng.

### 5. Quản lý cấu hình thiết bị

Đối tượng: device_config  
Công việc: Lưu cấu hình do người dùng thiết lập  
Biến lưu: `H_soil_threshold`, `watering_schedule`, `auto_mode`, `manual_cmd`  
Giải thích: Cấu hình này được cloud gửi ngược về ESP32 để thay đổi cách hệ thống tưới.

### 6. Gửi dữ liệu cho dashboard

Đối tượng: API / WebSocket  
Công việc: Cung cấp dữ liệu realtime và lịch sử cho giao diện  
Biến gửi: dữ liệu cảm biến, trạng thái bơm, log tưới, cảnh báo  
Giải thích: Dashboard dùng dữ liệu này để hiển thị trạng thái hệ thống cho người dùng.

## Layer 6: Tầng giao diện người dùng

### Khung tổng quan

Layer 6. Dashboard & User Layer  
Gồm: Web/App dashboard  
Công việc: Hiển thị dữ liệu, nhận cấu hình từ người dùng và gửi cảnh báo  
Input: dữ liệu từ cloud  
Output: `user_config`, `manual_cmd`, `notification`

### 1. Hiển thị dữ liệu thời gian thực

Đối tượng: Dashboard  
Công việc: Hiển thị trạng thái hiện tại của hệ thống  
Dữ liệu hiển thị: `T_air`, `H_air`, `H_soil`, `pump_state`, `cloud_status`  
Giải thích: Người dùng quan sát được nhiệt độ, độ ẩm và trạng thái bơm tại thời điểm hiện tại.

### 2. Hiển thị lịch sử và biểu đồ

Đối tượng: Dashboard chart  
Công việc: Vẽ biểu đồ dữ liệu theo thời gian  
Dữ liệu dùng: lịch sử độ ẩm đất, lịch sử nhiệt độ, lịch sử tưới  
Giải thích: Biểu đồ giúp người dùng theo dõi xu hướng khô/ẩm của đất và hiệu quả tưới.

### 3. Cấu hình ngưỡng tưới

Đối tượng: Form cấu hình  
Công việc: Người dùng nhập ngưỡng độ ẩm đất  
Biến tạo: `H_soil_threshold`  
Giải thích: Khi độ ẩm đất thấp hơn ngưỡng, hệ thống có thể tự động bật bơm.

### 4. Cấu hình lịch tưới

Đối tượng: Form lịch tưới  
Công việc: Người dùng đặt thời gian tưới theo lịch  
Biến tạo: `watering_schedule`, `watering_duration`  
Giải thích: Hệ thống có thể tưới theo thời điểm cố định ngoài cơ chế tự động theo độ ẩm.

### 5. Điều khiển thủ công

Đối tượng: Nút ON/OFF trên dashboard  
Công việc: Người dùng bật hoặc tắt bơm từ xa  
Biến tạo: `manual_cmd`, `auto_mode`  
Giải thích: Khi cần tưới ngay hoặc dừng bơm khẩn cấp, người dùng có thể gửi lệnh thủ công.

### 6. Hiển thị cảnh báo

Đối tượng: Notification  
Công việc: Thông báo lỗi hoặc trạng thái bất thường  
Dữ liệu dùng: `alert_event`, `Error_Flag`, `actuator_error`  
Giải thích: Người dùng nhận cảnh báo khi cảm biến lỗi, đất quá khô hoặc bơm hoạt động bất thường.

## Luồng dữ liệu tổng quát

Layer 1 tạo `SensorData_t`  
Layer 2 xử lý và tạo `ControlData_t`  
Layer 3 nhận `pump_cmd` để điều khiển bơm  
Layer 4 đóng gói `TelemetryPacket_t` và gửi lên cloud  
Layer 5 lưu dữ liệu, xử lý cảnh báo và quản lý cấu hình  
Layer 6 hiển thị dữ liệu và cho phép người dùng điều khiển hệ thống  
Layer 6 gửi `user_config` về Layer 5  
Layer 5 gửi `remote_config` về Layer 4  
Layer 4 chuyển thành `ConfigData_t` cho Layer 2
