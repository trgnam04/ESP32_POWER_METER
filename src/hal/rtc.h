#ifndef __RTC_H_
#define __RTC_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "time.h"
#include "sys/time.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void rtc_init(void);
void rtc_set(time_t timestamp);
time_t rtc_get(void);
String rtc_get_formated(void);
uint8_t rtc_update_from_server(const char* time_str);


#endif // __RTC_H_