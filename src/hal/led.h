#ifndef __LED_H_
#define __LED_H_


/* Includes ------------------------------------------------------------------*/

/* Define --------------------------------------------------------------------*/
#define LED_1_PIN 2
#define LED_2_PIN 10

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void led_init(void);
void led_process(void);

void led_1_on(void);
void led_1_off(void);
void led_1_toggle(unsigned long delay);

void led_2_on(void);
void led_2_off(void);
void led_2_toggle(unsigned long delay);

#endif // __LED_H_