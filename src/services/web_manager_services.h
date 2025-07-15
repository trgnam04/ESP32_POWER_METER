#ifndef __WEB_MANAGER_SERVICES_H_
#define __WEB_MANAGER_SERVICES_H_

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "html/html.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/
// Định danh nguồn của sự kiện
typedef enum {
    EVT_SRC_HTTP_SERVER,
    EVT_SRC_WEBSOCKET
} event_source_t;

// Định danh loại sự kiện cụ thể
typedef enum {
    // Sự kiện từ HTTP Server
    HTTP_REQ_UPDATE_WIFI,
    HTTP_REQ_UPDATE_ADMIN,
    HTTP_REQ_CHECK_LOGIN,
    HTTP_REQ_GO_NORMAL,
    
    // Sự kiện từ WebSocket
    WEBSOCKET_CONNECTED,
    WEBSOCKET_DISCONNECTED,
    WEBSOCKET_TEXT_RECEIVED
} event_type_t;

// Cấu trúc dữ liệu cho từng loại sự kiện
typedef struct {
    char param1[64];
    char param2[64];
} http_request_data_t;

typedef struct {
    uint8_t client_num;
    char    payload[128]; // Giả định payload tối đa 128 bytes
} websocket_data_t;

// Cấu trúc sự kiện hợp nhất - "Phong bì" chung
typedef struct {
    event_source_t source; // Nguồn gốc sự kiện
    event_type_t   type;   // Loại sự kiện là gì

    union {
        http_request_data_t http;
        websocket_data_t    ws;
    } data; // Dữ liệu đi kèm, dùng chung vùng nhớ
} app_event_t;

/* Variables -----------------------------------------------------------------*/
AsyncWebServer          g_http_server(80);
WebSocketsServer        g_ws_server(81);

static QueueHandle_t    g_event_queue;

static char             g_ap_ssid_temp[32];
static uint8_t          g_ap_logged_in;
static unsigned long    g_ap_logged_in_timeout;
static unsigned long    g_timeout;

/* Functions -----------------------------------------------------------------*/
void start_ap_mode();
void setup_http_server_endpoints();
void websocket_event_callback(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void task_event_handler(void *pvParameters);
void task_ap_main_loop(void *pvParameters);


#endif // __WEB_MANAGER_SERVICES_H_