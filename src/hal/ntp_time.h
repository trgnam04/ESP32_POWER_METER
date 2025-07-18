#ifndef NTP_TIME_H
#define NTP_TIME_H

#include "common.h"
#include <stddef.h> // Để có kiểu size_t
#include <time.h> // Thư viện thời gian chuẩn của C

/**
 * @brief Khởi tạo và cấu hình dịch vụ thời gian NTP.
 * Hàm này chỉ cần gọi một lần khi có kết nối Wi-Fi.
 */
void ntp_time_init();

/**
 * @brief Kiểm tra xem thời gian hệ thống đã được đồng bộ hay chưa.
 * @return true nếu thời gian đã được đồng bộ, ngược lại trả về false.
 */
bool ntp_time_is_synced();

/**
 * @brief Lấy thời gian hiện tại và định dạng thành chuỗi.
 * @param buffer Con trỏ đến bộ đệm để lưu chuỗi thời gian.
 * @param len Kích thước của bộ đệm.
 * @return true nếu lấy và định dạng thời gian thành công, ngược lại trả về false.
 */
bool ntp_time_get_string(char* buffer, size_t len);

#endif // NTP_TIME_H