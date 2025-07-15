#include "web_manager_services.h"

QueueHandle_t    g_event_queue;

char             g_ap_ssid_temp[32]         = "" ;
uint8_t          g_ap_logged_in             = 0 ;
unsigned long    g_ap_logged_in_timeout     = 0 ;
unsigned long    g_timeout                  = 0 ;

void start_ap_mode()
{
    g_event_queue = xQueueCreate(10, sizeof(app_event_t));
    if(g_event_queue == NULL)
    {
        // log
        return;
    }

    xTaskCreatePinnedToCore(
        task_event_handler,
        "EVENT HANDLER TASK",
        1024 * 5,
        NULL,
        5,
        NULL,
        1
    );

    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(WIFI_SSID_DEFAULT, WIFI_PASSWORD_DEFAULT);
    // log default wifi information

    IPAddress IP = WiFi.softAPIP();

    // log ip



    


}

void setup_http_server_endpoints()
{

}


void websocket_event_callback(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

}

void task_event_handler(void *pvParameters)
{

}