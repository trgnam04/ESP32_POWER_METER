/* Includes ------------------------------------------------------------------*/
#include "app_logic.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*                  TAG                     = "APP LOGIC";
TaskHandle_t                        _app_logic_handler_t    = NULL;
/* Functions -----------------------------------------------------------------*/
void init_normal_mode_app_logic(void)
{
    APP_LOGI(TAG, "Application Task init");    

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_normal_mode_app_logic,
        "APPLICATIPON LOGIC",
        1024 * 15,
        NULL,
        25,
        &_app_logic_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create Application Logic Task failed.");

}

void task_normal_mode_app_logic(void *pvParameters)
{
    APP_LOGI(TAG, "Application Logic Task started.");
    TickType_t xLastSendTime = 0;
    const TickType_t xSendInterval = pdMS_TO_TICKS(60000); // Gửi dữ liệu mỗi 60 giây    
    
    init_wifi_manager();
    vTaskDelay(100);
    init_mqtt_client();

    APP_LOGI(TAG, "Load all configuration");
    APP_LOGI(TAG, "WiFi SSID: %s, Password: %s", _wifi_ssid, _wifi_password);
    APP_LOGI(TAG, "Device ID: %s, Station code: %s", _id, _station_code);
    APP_LOGI(TAG, "MQTT Host: %s, MQTT username: %s, MQTT password", _mqtt_host, _mqtt_username, _mqtt_password);
    APP_LOGI(TAG, "MQTT Topic Subscribed: %s, MQTT Publish: %s", _mqtt_topic_sub, _mqtt_topic_pub);


    for(;;) {
        // 1. Xử lý lệnh đến từ MQTT (non-blocking)
        mqtt_message_t incoming_cmd;
        if (xQueueReceive(_mqtt_incoming_queue, &incoming_cmd, 0) == pdPASS) {
            APP_LOGI(TAG, "Received command on topic %s", incoming_cmd.topic);
            if (strcmp(incoming_cmd.topic, _mqtt_topic_sub) == 0) {
                // Phân tích JSON từ payload và điều khiển relay
                // ... ví dụ: relay_control(json["state"]); ...
            }
        }

        // 2. Chuẩn bị và gửi dữ liệu điện năng định kỳ
        if ((xTaskGetTickCount() - xLastSendTime) >= xSendInterval) {
            // Chờ đến khi MQTT sẵn sàng
            xEventGroupWaitBits(_normal_mode_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
            
            // Đọc cảm biến, tạo chuỗi JSON
            // ... String json_payload = create_electricity_json(); ...
            String json_payload = ""; // for dummy
            
            // Đóng gói và gửi vào hàng đợi đi
            mqtt_message_t outgoing_data;
            strcpy(outgoing_data.topic, _mqtt_topic_pub);
            strcpy(outgoing_data.payload, json_payload.c_str());
            xQueueSend(_mqtt_outgoing_queue, &outgoing_data, 0);
            
            xLastSendTime = xTaskGetTickCount(); // Reset bộ đếm thời gian
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}