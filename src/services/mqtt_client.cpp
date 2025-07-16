/* Includes ------------------------------------------------------------------*/
#include "mqtt_client.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*                  TAG                         = "MQTT CLIENT";
TaskHandle_t                        _mqtt_client_handler_t      = NULL;


/* Functions -----------------------------------------------------------------*/
void init_mqtt_client(void)
{
    APP_LOGI(TAG, "Mqtt Client Task init.");

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_mqtt_client,
        "MQTT CLIENT",
        1024 * 10,
        NULL,
        1,
        &_mqtt_client_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create Mqtt Client Task failed.");
}

void mqtt_message_callback(String &topic, String &payload)
{    
    mqtt_message_t msg;
    strncpy(msg.topic, topic.c_str(), sizeof(msg.topic)-1);
    strncpy(msg.payload, payload.c_str(), sizeof(msg.payload)-1);
    xQueueSend(_mqtt_incoming_queue, &msg, 0);
}

void task_mqtt_client(void *pvParameters) {
    APP_LOGI(TAG, "MQTT Client Task started.");
    MQTTClient mqtt_client(1024);
    WiFiClient wifi_client;

    memory_save_mqtt_config();
    memory_save_device_id();

    memory_load_mqtt_config();
    memory_load_device_id();    
    
    
    for(;;) {
        // 1. Chờ đến khi có Wi-Fi
        APP_LOGI(TAG, "MQTT: Waiting for WiFi connection...");
        xEventGroupWaitBits(_normal_mode_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        
        // 2. Kết nối MQTT
        APP_LOGI(TAG, "MQTT: WiFi connected, now connecting to MQTT broker...");
        mqtt_client.begin(_mqtt_host, _mqtt_port, wifi_client);
        mqtt_client.onMessage(mqtt_message_callback);
        
        // Vòng lặp kết nối lại MQTT
        while(!mqtt_client.connect(_id, _mqtt_username, _mqtt_password)) {
            APP_LOGW(TAG, "MQTT connection failed, retrying in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        APP_LOGI(TAG, "MQTT Connected!");
        xEventGroupSetBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);

        // 3. Đăng ký các topic cần thiết
        mqtt_client.subscribe(_mqtt_topic_sub);

        // 4. Vòng lặp hoạt động
        while(mqtt_client.connected()) {
            mqtt_client.loop(); // Xử lý các tin nhắn đến (sẽ gọi callback)

            // Kiểm tra queue dữ liệu đi và gửi
            mqtt_message_t data_to_send;
            if (xQueueReceive(_mqtt_outgoing_queue, &data_to_send, 0) == pdPASS) {
                mqtt_client.publish(data_to_send.topic, data_to_send.payload);
                APP_LOGI(TAG, "MQTT: Published data to topic %s", data_to_send.topic);
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }

        // Nếu vòng lặp trên bị thoát ra (do mất kết nối), báo cho hệ thống
        xEventGroupClearBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);
        APP_LOGW(TAG, "MQTT Disconnected. Will retry...");
    }
}