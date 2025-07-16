#ifndef __BUTTON_H__
#define __BUTTON_H__
/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_assert.h"
#include "app_log.h"

/* Define --------------------------------------------------------------------*/
#define BUTTON_PIN                  0
#define BUTTON_DEBOUNCE_MS          50
#define BUTTON_SHORT_PRESS_TICK     20   // 1s nếu task delay 50ms
#define BUTTON_LONG_PRESS_TICK      100  // 5s nếu task delay 50ms

/* Functions -----------------------------------------------------------------*/
void button_init(void);
void button_process(void);

bool button_is_short_pressed(void);
bool button_is_long_pressed(void);
uint32_t button_get_press_duration(void);

#endif
