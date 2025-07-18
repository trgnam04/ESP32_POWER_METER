/* Includes ------------------------------------------------------------------*/
#include "mqtt_client.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
char                                _mqtt_host[65]              = MQTT_HOST;
char                                _mqtt_username[65]          = MQTT_USERNAME;
char                                _mqtt_password[65]          = MQTT_PASSWORD;
char                                _mqtt_topic_sub[65]         = MQTT_TOPIC_SUB;
char                                _mqtt_topic_pub[65]         = MQTT_TOPIC_PUB;
uint32_t                            _mqtt_port                  = MQTT_PORT;
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
    APP_LOGI(TAG, "Message from server.");
    mqtt_message_t msg;
    strncpy(msg.topic, topic.c_str(), sizeof(msg.topic) - 1);
    strncpy(msg.payload, payload.c_str(), sizeof(msg.payload) - 1);
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
        APP_LOGI(TAG, "MQTT: Waiting for WiFi connection...");
        xEventGroupWaitBits(_normal_mode_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
                
        APP_LOGI(TAG, "MQTT: WiFi connected, now connecting to MQTT broker...");
        mqtt_client.begin(_mqtt_host, _mqtt_port, wifi_client);
        mqtt_client.onMessage(mqtt_message_callback);
                
        while(!mqtt_client.connect(_id, _mqtt_username, _mqtt_password)) {
            APP_LOGW(TAG, "MQTT connection failed, retrying in 5 seconds...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        APP_LOGI(TAG, "MQTT Connected!");
        xEventGroupSetBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);
        
        if(mqtt_client.subscribe(_mqtt_topic_sub))
        {
            APP_LOGI(TAG, "Subcribe topic %s successful", _mqtt_topic_sub);
        }
        else
        {
            APP_LOGW(TAG, "Subcribe topic %s failed.", _mqtt_topic_sub);
        }

        while(mqtt_client.connected()) {
            mqtt_client.loop(); 
            
            mqtt_message_t data_to_send;
            if (xQueueReceive(_mqtt_outgoing_queue, &data_to_send, 0) == pdPASS) {
                mqtt_client.publish(data_to_send.topic, data_to_send.payload);
                APP_LOGI(TAG, "MQTT: Published data to topic %s", data_to_send.topic);
            }
            
            mqtt_message_t data_receive;
            if (xQueueReceive(_mqtt_incoming_queue, &data_receive, 0) == pdPASS) {
                APP_LOGI(TAG, "MQTT: Received command from server: %s", data_receive.payload);

                // Parse JSON string
                cJSON *json = cJSON_Parse(data_receive.payload);
                if (!json) {
                    APP_LOGW(TAG, "MQTT: Invalid JSON payload");
                    return;
                }

                cJSON *cmd = cJSON_GetObjectItem(json, "cmd");
                if (!cmd || !cJSON_IsString(cmd)) {
                    APP_LOGW(TAG, "MQTT: 'cmd' field missing or invalid");
                    cJSON_Delete(json);
                    return;
                }

                if (strcmp(cmd->valuestring, "on") == 0) {
                    APP_LOGI(TAG, "MQTT: Turning device ON");
                    relay_on();  // Hàm tự định nghĩa
                } 
                else if (strcmp(cmd->valuestring, "off") == 0) 
                {
                    APP_LOGI(TAG, "MQTT: Turning device OFF");
                    relay_off(); // Hàm tự định nghĩa
                } 
                // else if (strcmp(cmd->valuestring, "on_for") == 0) 
                // {
                //     cJSON *duration = cJSON_GetObjectItem(json, "duration");
                //     if (duration && cJSON_IsNumber(duration)) {
                //         APP_LOGI(TAG, "MQTT: Turning device ON for %d seconds", duration->valueint);
                //         relay_on();
                //         // Tạo task riêng nếu cần delay không chặn
                //         vTaskDelay(pdMS_TO_TICKS(duration->valueint * 1000));
                //         relay_off();
                //     } else {
                //         APP_LOGW(TAG, "MQTT: Missing or invalid duration");
                //     }
                // } 
                else 
                {
                    APP_LOGW(TAG, "MQTT: Unknown command: %s", cmd->valuestring);
                }

                cJSON_Delete(json);
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
        
        xEventGroupClearBits(_normal_mode_event_group, MQTT_CONNECTED_BIT);
        APP_LOGW(TAG, "MQTT Disconnected. Will retry...");
    }
}


