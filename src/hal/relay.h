#ifndef __RELAY_H_
#define __RELAY_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Define --------------------------------------------------------------------*/
#define RELAY_PIN 4

/* Variables -----------------------------------------------------------------*/
extern uint8_t relay_status;

/* Functions -----------------------------------------------------------------*/
void relay_init(void);
void relay_process(void);

void relay_on(void);
void relay_off(void);

#endif // __RELAY_H_