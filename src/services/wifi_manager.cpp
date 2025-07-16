/* Includes ------------------------------------------------------------------*/
#include "wifi_manager.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*          TAG                         = "WIFI MANAGER";
TaskHandle_t                _wifi_manager_handler_t     = NULL;

/* Functions -----------------------------------------------------------------*/
void init_wifi_manager(void)
{
    APP_LOGI(TAG, "WiFi Manager Task init.");

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_wifi_manager,
        "WIFI MANAGER",        
        1024 * 10,
        NULL,
        1,
        &_wifi_manager_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create WiFi Manager Task failed.");
}

void wifi_event_callback(WiFiEvent_t event)
{
    switch(event)
    {
        case SYSTEM_EVENT_STA_CONNECTED:
        {            
            break;
        }
        case SYSTEM_EVENT_STA_GOT_IP:
        {
            APP_LOGI(TAG, "WiFi Connected, IP: %s", WiFi.localIP().toString().c_str());
            xEventGroupSetBits(_normal_mode_event_group, WIFI_CONNECTED_BIT);
            break;
        }
        case SYSTEM_EVENT_STA_DISCONNECTED:
        {
            APP_LOGW(TAG, "WiFi Disconnected");
            xEventGroupClearBits(_normal_mode_event_group, WIFI_CONNECTED_BIT);
            break;
        }
        default: 
            break;
    }
}

void task_wifi_manager(void *pvParameters)
{
    APP_LOGI(TAG, "WiFi Manager Task started.");

    memory_load_wifi_config();

    WiFi.onEvent(wifi_event_callback);
    WiFi.mode(WIFI_STA);
    WiFi.begin(_wifi_ssid, _wifi_password); 

    for(;;) {
        // Task này không cần làm gì nhiều trong vòng lặp vì đã có event callback.
        // Nó chỉ tồn tại để khởi tạo và có thể xử lý các logic phức tạp hơn nếu cần.
        // Nếu WiFi.reconnect() không hoạt động tốt, bạn có thể đặt logic reconnect ở đây.
        if (WiFi.status() != WL_CONNECTED) {
            WiFi.reconnect();
        }
        vTaskDelay(pdMS_TO_TICKS(10000)); // Kiểm tra lại sau mỗi 10 giây
    }


}