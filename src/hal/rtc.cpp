/* Includes ------------------------------------------------------------------*/
#include "rtc.h"


/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static tm rtc_time;


/* Functions -----------------------------------------------------------------*/
void rtc_init(void)
{
  time_t now = 0;
  time(&now);
  localtime_r(&now, &rtc_time);
}

void rtc_set(time_t timestamp)
{
  localtime_r(&timestamp, &rtc_time);
}

time_t rtc_get(void)
{
  return mktime(&rtc_time);
}

String rtc_get_formated(void)
{
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%04d/%02d/%02dT%02d:%02d:%02d",
            rtc_time.tm_year + 1900,
            rtc_time.tm_mon + 1,
            rtc_time.tm_mday,
            rtc_time.tm_hour,
            rtc_time.tm_min,
            rtc_time.tm_sec);
  return String(buffer);
}

uint8_t rtc_update_from_server(const char* time_str)
{
  int year, month, day, hour, min, sec;
  if (sscanf(time_str, "%d/%d/%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec) != 6) {
    return false;
  }

  rtc_time.tm_year = year - 1900;
  rtc_time.tm_mon  = month - 1;
  rtc_time.tm_mday = day;
  rtc_time.tm_hour = hour;
  rtc_time.tm_min  = min;
  rtc_time.tm_sec  = sec;

  return true;

}
