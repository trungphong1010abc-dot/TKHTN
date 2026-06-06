# Mô tả các layer tiếp theo

File flow: `flow-cac-layer-tiep-theo.svg`

## Layer 3: Actuator & Local Feedback

Chức năng chính của Layer 3 là nhận lệnh `pump_cmd` từ Layer 2 và điều khiển phần cứng chấp hành gồm relay và máy bơm. Khi `pump_cmd = ON`, relay được kích hoạt để máy bơm chạy trong khoảng thời gian `duration`. Khi hết thời gian tưới hoặc khi `pump_cmd = OFF`, relay được tắt để dừng bơm.

Output của layer này gồm:

- `pump_state`: trạng thái thực tế của máy bơm.
- `watering_time`: thời gian tưới đã thực hiện.
- `actuator_error`: cờ lỗi chấp hành nếu bơm/relay hoạt động bất thường.

Các dữ liệu phản hồi này được gửi ngược về Layer 2 để đóng gói vào log điều khiển.

## Layer 4: Communication & Cloud Gateway

Layer 4 chạy trên ESP32, phụ trách đóng gói dữ liệu và truyền lên cloud. Nguồn dữ liệu chính là `SensorData_t` từ Layer 1 và `ControlData_t` từ Layer 2.

Các bước chính:

1. Tạo `TelemetryPacket_t` gồm `device_id`, `timestamp`, dữ liệu cảm biến, quyết định điều khiển và trạng thái bơm.
2. Kiểm tra trạng thái WiFi.
3. Nếu có mạng, gửi dữ liệu bằng MQTT hoặc HTTP.
4. Nếu mất mạng, lưu tạm dữ liệu vào buffer và retry ở chu kỳ sau.
5. Nhận `remote_config` từ cloud và chuyển thành `ConfigData_t` gửi về Layer 2.

Các biến nên có:

- `cloud_status`
- `retry_count`
- `TelemetryPacket_t`
- `ConfigData_t`

## Layer 5: Cloud Storage & Application Service

Layer 5 lưu trữ dữ liệu và xử lý nghiệp vụ phía cloud.

Các nhóm dữ liệu chính:

- `telemetry_log`: lưu nhiệt độ, độ ẩm không khí, độ ẩm đất, trạng thái bơm, lỗi cảm biến và thời gian.
- `device_config`: lưu ngưỡng tưới, lịch tưới, chế độ tự động/thủ công.
- `alert_event`: lưu các sự kiện cảnh báo như đất quá khô, cảm biến lỗi hoặc bơm chạy quá lâu.

Layer này cũng có thể tính các thống kê như độ ẩm trung bình theo ngày, tổng thời gian tưới và số lần bơm hoạt động.

## Layer 6: Dashboard, User & Notification

Layer 6 là giao diện để người dùng quan sát và cấu hình hệ thống.

Chức năng chính:

- Hiển thị nhiệt độ, độ ẩm không khí, độ ẩm đất.
- Hiển thị trạng thái bơm và lịch sử tưới.
- Cho phép người dùng chỉnh ngưỡng độ ẩm đất.
- Cho phép đặt lịch tưới.
- Cho phép bật/tắt bơm thủ công.
- Gửi cảnh báo khi hệ thống có lỗi hoặc điều kiện bất thường.

## Luồng phản hồi cấu hình

Luồng phản hồi từ người dùng về thiết bị:

1. Người dùng chỉnh cấu hình trên Dashboard.
2. Layer 6 gửi cấu hình mới lên Layer 5.
3. Layer 5 lưu vào `device_config` và tạo `remote_config`.
4. Layer 4 trên ESP32 nhận `remote_config`.
5. Layer 4 kiểm tra version/checksum rồi chuyển thành `ConfigData_t`.
6. Layer 2 cập nhật ngưỡng, lịch tưới hoặc lệnh thủ công.

Luồng này giúp hệ thống không chỉ gửi dữ liệu một chiều lên cloud mà còn nhận điều khiển từ xa.
