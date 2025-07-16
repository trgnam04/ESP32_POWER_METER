/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "app_log.h"
#include "app_assert.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
void init_mqtt_client(void);
void mqtt_message_callback(String &topic, String &payload);
void task_mqtt_client(void *pvParameters);