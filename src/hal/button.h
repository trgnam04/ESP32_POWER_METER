#ifndef __BUTTON_H_
#define __BUTTON_H_


/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Define --------------------------------------------------------------------*/
#define BUTTON_BOOT_PIN 0 // 5

/* Variables -----------------------------------------------------------------*/
extern uint32_t key_code;
extern uint32_t key_time_before_releasing;

/* Functions -----------------------------------------------------------------*/
void button_init(void);
void button_process(void);

#endif // __BUTTON_H_