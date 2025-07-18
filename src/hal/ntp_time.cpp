#include "ntp_time.h"

// Múi giờ Việt Nam (GMT+7)
static const char* ntp_server = "pool.ntp.org";
static const long  gmt_offset_sec = 7 * 3600; // 7 giờ * 3600 giây/giờ
static const int   daylight_offset_sec = 0;   // Không có giờ mùa hè

static bool is_time_synced = false;

void ntp_time_init() {
    // Cấu hình client SNTP chạy nền
    // Hàm này sẽ tự động đồng bộ thời gian khi có kết nối internet.
    configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
    is_time_synced = false; // Đặt lại cờ khi khởi tạo
}

bool ntp_time_is_synced() {
    return is_time_synced;
}

bool ntp_time_get_string(char* buffer, size_t len) {
    struct tm timeinfo;

    // getLocalTime là non-blocking, nó trả về false nếu thời gian chưa được đồng bộ
    if (!getLocalTime(&timeinfo)) {
        snprintf(buffer, len, "Time not synced");
        is_time_synced = false;
        return false;
    }

    // Nếu lọt vào đây, nghĩa là đã lấy được thời gian
    is_time_synced = true;

    // Sử dụng strftime để định dạng chuỗi một cách an toàn và chuẩn xác
    // %Y: Năm đầy đủ (ví dụ: 2025)
    // %m: Tháng (01-12)
    // %d: Ngày (01-31)
    // %H: Giờ (00-23)
    // %M: Phút (00-59)
    // %S: Giây (00-59)
    strftime(buffer, len, "%Y/%m/%dT%H:%M:%S", &timeinfo);

    return true;
}