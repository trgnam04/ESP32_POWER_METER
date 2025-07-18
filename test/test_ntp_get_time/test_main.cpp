#include <Arduino.h>
#include <unity.h>
#include "rtc.h"
#include "ntp_time.h"

void setup_wifi()
{
    WiFi.begin("BKIT_CS2", "cselabc5c6");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry++ < 10) {
        delay(1000);
    }
}

void test_ntp_time_valid() {
    struct tm timeinfo;
    uint8_t success = ntp_time_get(&timeinfo);
    TEST_ASSERT_TRUE_MESSAGE(success, "Failed to get time from NTP");
    TEST_ASSERT_GREATER_THAN_INT(2020 - 1900, timeinfo.tm_year);
}

void test_ntp_time_string_format() {
    char buffer[32];
    ntp_time_get_string(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("Invalid Time", buffer));
    TEST_ASSERT_EQUAL(strlen("YYYY/MM/DDTHH:MM:SS"), strlen(buffer));
}

void test_rtc_sync_with_ntp() {
    char buffer[32];
    ntp_time_get_string(buffer, sizeof(buffer));

    uint8_t updated = rtc_update_from_server(buffer);
    TEST_ASSERT_TRUE_MESSAGE(updated, "RTC update from NTP string failed");

    String rtc_time_str = rtc_get_formated();
    TEST_ASSERT_EQUAL_STRING(buffer, rtc_time_str.c_str());
}

void test_rtc_set_and_get_consistency() {
    time_t now = time(NULL);
    rtc_set(now);
    time_t rtc_now = rtc_get();

    // Cho phép sai số nhẹ do xử lý
    TEST_ASSERT_INT_WITHIN(2, now, rtc_now);
}

void setup() {
    UNITY_BEGIN();
    setup_wifi();
    delay(1000);
    ntp_time_init();       // Khởi tạo đồng bộ NTP
    delay(2000);           // Đợi thời gian được cập nhật

    RUN_TEST(test_ntp_time_valid);
    RUN_TEST(test_ntp_time_string_format);
    RUN_TEST(test_rtc_sync_with_ntp);
    RUN_TEST(test_rtc_set_and_get_consistency);

    UNITY_END();
}

void loop() {}
