#ifndef __WEB_MANAGER_SERVICES_H_
#define __WEB_MANAGER_SERVICES_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "memory_config.h"
#include "html/html.h"

/* Define --------------------------------------------------------------------*/
#define AP_TIMEOUT_MS (5 * 50 * 1000)
#define LOGIN_TIMEOUT_MS (30 * 60 * 1000)

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
extern AsyncWebServer   g_http_server;
extern WebSocketsServer g_ws_server;


/* Functions -----------------------------------------------------------------*/
void start_ap_mode();
void setup_http_server_endpoints();
void websocket_event_callback(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void task_event_handler(void *pvParameters);
void task_ap_main_loop(void *pvParameters);


#endif // __WEB_MANAGER_SERVICES_H_