#ifndef __BUZZER_H_
#define __BUZZER_H_

 
/* Includes ------------------------------------------------------------------*/
#include "common.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
typedef struct {
	uint16_t frequency;
	uint16_t  duration;
} My_ToneTypeDef;

/* Functions -----------------------------------------------------------------*/
extern My_ToneTypeDef tones_5beep[];
extern My_ToneTypeDef tones_SMB[];
extern My_ToneTypeDef tones_startup[];
extern My_ToneTypeDef tones_turnoff[];
extern My_ToneTypeDef tones_3beep[];
extern My_ToneTypeDef tones_2_beep[];
extern My_ToneTypeDef single_beep[];
void buzzer_init();

void test_buzzer();
void play_a_tone(My_ToneTypeDef* input_tone);
void buzzer_process();

#endif // __BUZZER_H_
