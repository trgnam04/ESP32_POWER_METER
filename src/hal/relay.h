#ifndef __RELAY_H_
#define __RELAY_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_log.h"

/* Define --------------------------------------------------------------------*/
#define RELAY_PIN                   4
#define RELAY_TOGGLE_DELAY          (500 / 50)
/* Variables -----------------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/
void relay_init(void);
void relay_process(void);
void relay_on(void);
void relay_off(void);
void relay_toggle(void);
uint8_t relay_get_status(void);
#endif // __RELAY_H_