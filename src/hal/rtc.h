#ifndef __RTC_H_
#define __RTC_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <WString.h>

#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
extern bool     rtc_state;
extern uint16_t rtc_year;
extern uint8_t  rtc_month, rtc_day;
extern uint8_t  rtc_hour, rtc_minute, rtc_second;
extern String   rtc_string;
extern TaskHandle_t    RTCTask_handle_t;

/* Functions -----------------------------------------------------------------*/
void rtc_init(void);
void rtc_process(void);

#endif // __RTC_H_