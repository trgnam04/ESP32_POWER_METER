/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "app_log.h"
#include "app_assert.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "ntp_time.h"
#include "rtc.h"
/* Define --------------------------------------------------------------------*/
#define SYNC_TIME_PERIOD (pdMS_TO_TICKS(60000))

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/

/* Functions -----------------------------------------------------------------*/
uint8_t prepare_mqtt_payload(String &mqtt_payload, raw_sensor_data_t* sensor_data, String timestamp);

void init_normal_mode_app_logic(void);
void task_normal_mode_app_logic(void *pvParameters);
void init_ntp_sync(void);
void task_ntp_sync(void *pvParameter);