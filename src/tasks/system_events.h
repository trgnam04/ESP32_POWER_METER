#ifndef __SYSTEM_EVENTS_H_
#define __SYSTEM_EVENTS_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "app_log.h"
#include "app_assert.h"
#include "web_manager_services.h"
#include "input_processing.h"
#include "app_logic.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/
void init_system_supervisor(void);
void task_system_supervisor(void *pvParameters);



#endif