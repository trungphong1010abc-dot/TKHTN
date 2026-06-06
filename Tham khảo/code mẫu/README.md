# Code mẫu theo flow layer

Các file trong thư mục này là code khung để bám theo flow, chưa phải code cuối cùng cho project.

## Danh sách file

- `common_types_layer_flow.h`: định nghĩa kiểu dữ liệu, enum và queue dùng chung.
- `00_khoi_tao_queue_layer_flow.c`: tạo queue nối giữa các task.
- `01_task_control_gui_lenh_bom.c`: Layer 2 tạo `pump_cmd`, gửi sang Layer 3 và nhận phản hồi.
- `02_task_actuator_dieu_khien_relay_bom.c`: Layer 3 nhận `pump_cmd`, điều khiển GPIO/AO3400/relay/máy bơm.
- `03_task_cloud_dong_goi_log.c`: Layer 4 nhận `ControlData_t` và đóng gói log gửi cloud.

## Đường truyền chính

```text
Task_Control
-> actuatorCmdQueue
-> Task_Actuator
-> GPIO ESP32
-> AO3400 / relay
-> Pump
-> actuatorFeedbackQueue
-> Task_Control
-> controlToCloudQueue
-> Task_Cloud
```

## Ghi chú

AO3400, relay và máy bơm là phần cứng, trong code chỉ điều khiển chân GPIO.
Khi code thật, cần thay các hàm giả lập GPIO/cloud bằng API đúng của board đang dùng.
