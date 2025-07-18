/* Includes ------------------------------------------------------------------*/
#include "relay.h"

/* Define --------------------------------------------------------------------*/
typedef enum
{
  RELAY_ON,
  RELAY_OFF,
  RELAY_TOGGLE
} E_RELAY_STATE;

/* Variables -----------------------------------------------------------------*/
static const char*        TAG                     = "RELAY";
static E_RELAY_STATE      relay_state             = RELAY_OFF;
static uint32_t           relay_toggle_delay      = RELAY_TOGGLE_DELAY;
static volatile uint8_t   relay_status            = 0; 

/* Functions -----------------------------------------------------------------*/
void relay_init(void)
{
  APP_LOGI(TAG, "Init Relay.");
  pinMode(RELAY_PIN, OUTPUT);
  relay_off();

  relay_status = 0;  
}

void relay_process(void)
{
  static unsigned long current_millis  = 0;
  static unsigned long previous_millis = 0;

  current_millis = millis();
  if (current_millis - previous_millis >= 50)
  {
    previous_millis = current_millis;

    switch (relay_state)
    {
      case RELAY_ON:
        digitalWrite(RELAY_PIN, LOW);
        break;

      case RELAY_OFF:
        digitalWrite(RELAY_PIN, HIGH);
        break;

      case RELAY_TOGGLE:
        if (--relay_toggle_delay == 0)
        {
          relay_toggle_delay = RELAY_TOGGLE_DELAY;

          digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
        }

        break;

      default:
        relay_state = RELAY_OFF;
        break;
    }
  }
}

void relay_on(void)
{
  relay_status       = 1;  
  digitalWrite(RELAY_PIN, relay_status);
  
}

void relay_off(void)
{
  relay_status       = 0;
  digitalWrite(RELAY_PIN, relay_status);
}

void relay_toggle(void)
{
  relay_status = !relay_status;
  digitalWrite(RELAY_PIN, relay_status);

}

uint8_t relay_get_status(void)
{
  return relay_status;
}