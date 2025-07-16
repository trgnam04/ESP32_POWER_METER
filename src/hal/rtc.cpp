/* Includes ------------------------------------------------------------------*/
#include "rtc.h"


/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
WiFiUDP    ntpUDP;
NTPClient  time_client(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);
time_t     epoch_time;
struct tm* ptm;

bool     rtc_state = false;
uint16_t rtc_year  = 0;
uint8_t  rtc_month = 0, rtc_day = 0;
uint8_t  rtc_hour = 0, rtc_minute = 0, rtc_second = 0;
String   rtc_string;

/* Functions -----------------------------------------------------------------*/
void rtc_init(void)
{
  time_client.begin();
}

void rtc_process(void)
{
  static unsigned long current_millis  = 0;
  static unsigned long previous_millis = 0;

  current_millis = millis();
  if (current_millis - previous_millis >= 1000)
  {
    previous_millis = current_millis;

    time_client.update();
    epoch_time = time_client.getEpochTime();

    if (time_client.isTimeSet())
    {
      rtc_state = true;

      rtc_hour   = time_client.getHours();
      rtc_minute = time_client.getMinutes();
      rtc_second = time_client.getSeconds();

      tm* ptm   = gmtime((time_t*)&epoch_time);
      rtc_day   = ptm->tm_mday;
      rtc_month = ptm->tm_mon + 1;
      rtc_year  = ptm->tm_year - 100;

      rtc_string = String(rtc_year) + "/" + (rtc_month < 10 ? "0" : "") + String(rtc_month) + "/" + (rtc_day < 10 ? "0" : "") + String(rtc_day) + "T" +
                          (rtc_hour < 10 ? "0" : "") + String(rtc_hour) + ":" + (rtc_minute < 10 ? "0" : "") + String(rtc_minute) + ":" +
                          (rtc_second < 10 ? "0" : "") + String(rtc_second);

      // Serial.print("rtc: \t [update] ");
      // Serial.println(rtc_string);
    }
    else
    {
      // Serial.println("rtc: \t [error] time not set");

      rtc_state = false;
    }
  }
}