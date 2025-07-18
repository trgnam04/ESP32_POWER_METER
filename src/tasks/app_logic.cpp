/* Includes ------------------------------------------------------------------*/
#include "app_logic.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*                  TAG                     = "APP LOGIC";
TaskHandle_t                        _app_logic_handler_t    = NULL;
TaskHandle_t                        _ntp_sync_handler_t     = NULL;
static char                         time_buffer[32];
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

void init_ntp_sync(void)
{
    APP_LOGI(TAG, "NTP Sync init"); 
    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_ntp_sync,
        "SYNC TIME",
        1024 * 2,
        NULL,
        1,
        &_ntp_sync_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create NTP Sync Task failed.");
}

void task_normal_mode_app_logic(void *pvParameters) {
    APP_LOGI(TAG, "Application Logic Task started.");
    TickType_t xLastSendTime = 0;
    const TickType_t xSendInterval = pdMS_TO_TICKS(60000); // Gửi dữ liệu mỗi 60 giây        

    raw_sensor_data_t received_data;

    for(;;) {
        // Chờ nhận dữ liệu thô từ task sensor
        if (xQueueReceive(_raw_sensor_data_queue, &received_data, portMAX_DELAY) == pdPASS) {

            
            if (!received_data.is_valid) {
             
                continue;
            }            

            if (_system_current_state == STATE_NORMAL_MODE) {
                
                xEventGroupWaitBits(_normal_mode_event_group, (MQTT_CONNECTED_BIT | NTP_SYNCED_BIT), pdFALSE, pdTRUE, portMAX_DELAY);

                char timestamp_buffer[32];
                ntp_time_get_string(timestamp_buffer, sizeof(timestamp_buffer));
                
                String mqtt_payload;
                prepare_mqtt_payload(mqtt_payload, &received_data, timestamp_buffer);
                
                
                mqtt_message_t mqtt_msg;
                strncpy(mqtt_msg.topic, _mqtt_topic_pub, sizeof(mqtt_msg.topic) - 1);
                strncpy(mqtt_msg.payload, mqtt_payload.c_str(), sizeof(mqtt_msg.payload) - 1);
                xQueueSend(_mqtt_outgoing_queue, &mqtt_msg, 0);

                APP_LOGI(TAG, "APP: Routed sensor data to MQTT task.");

            } else if (_system_current_state == STATE_AP_MODE) {
                

                String mqtt_payload;
                prepare_mqtt_payload(mqtt_payload, &received_data, "");
                                                
                
                app_event_t sensor_evt;
                sensor_evt.source = EVT_SRC_SENSOR; 
                sensor_evt.type = SENSOR_DATA_READY; 
                strncpy(sensor_evt.data.sensor.payload, mqtt_payload.c_str(), sizeof(sensor_evt.data.sensor.payload) - 1);
                xQueueSend(_web_manager_event_queue, &sensor_evt, 0);

                APP_LOGI(TAG, "APP: Routed sensor data to Web Event Handler.");
            }
        }
    }
}

void task_ntp_sync(void *pvParameters) {
    bool isFirstSync = true;
    const TickType_t xRetryInterval = pdMS_TO_TICKS(15000); 
    const TickType_t xLongDelay = pdMS_TO_TICKS(1 * 60 * 60 * 1000); 

    APP_LOGI(TAG, "NTP Sync Task started.");

    for (;;) {
        
        APP_LOGI(TAG, "NTP: Waiting for WiFi connection...");
        xEventGroupWaitBits(_normal_mode_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
        
        
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        
        IPAddress resolvedIP;
        if (!WiFi.hostByName(NTP_SERVER, resolvedIP)) { 
            APP_LOGE(TAG, "NTP: DNS resolution failed. Retrying in 15 seconds...");
            vTaskDelay(xRetryInterval);
            continue; 
        }
        APP_LOGI(TAG, "NTP: DNS OK. '%s' is at %s", NTP_SERVER, resolvedIP.toString().c_str());

        
        ntp_time_init();

        
        if (ntp_time_get_string(time_buffer, sizeof(time_buffer))) {
            APP_LOGI(TAG, "NTP: Time synced successfully: %s", time_buffer);
            
            if (isFirstSync) {
                xEventGroupSetBits(_normal_mode_event_group, NTP_SYNCED_BIT);
                isFirstSync = false;
            }

            
            vTaskDelay(xLongDelay);

        } else {
            APP_LOGW(TAG, "NTP: Failed to sync time. Retrying in 15 seconds...");
            vTaskDelay(xRetryInterval);
        }
    }
}

uint8_t prepare_mqtt_payload(String &mqtt_payload, raw_sensor_data_t* sensor_data, String timestamp)
{
    JsonDocument json_doc;
    JsonObject   json_root = json_doc.to<JsonObject>();

    json_root["dt"] = timestamp;

    // Voltage
    JsonObject voltageObj = json_root["voltage"].to<JsonObject>();
    voltageObj["name"]    = "Voltage";
    voltageObj["value"]   = sensor_data->voltage;
    voltageObj["unit"]    = "V";

    // Current
    JsonObject currentObj = json_root["current"].to<JsonObject>();
    currentObj["name"]    = "Current";
    currentObj["value"]   = sensor_data->current;
    currentObj["unit"]    = "A";

    // Active Power
    JsonObject activePowerObj = json_root["activePower"].to<JsonObject>();
    activePowerObj["name"]    = "Active Power";
    activePowerObj["value"]   = sensor_data->activePower;
    activePowerObj["unit"]    = "W";

    // Active Energy
    JsonObject activeEnergyObj = json_root["activeEnergy"].to<JsonObject>();
    activeEnergyObj["name"]    = "Active Energy";
    activeEnergyObj["value"]   = sensor_data->activeEnergy;
    activeEnergyObj["unit"]    = "kWh";

    // Power Factor
    if(sensor_data->current == 0)
    {
        JsonObject powerFactorObj = json_root["powerFactor"].to<JsonObject>();
        powerFactorObj["name"]    = "Power Factor";
        powerFactorObj["value"]   = 100;
        powerFactorObj["unit"]    = "%";
    }
    else
    {
        JsonObject powerFactorObj = json_root["powerFactor"].to<JsonObject>();
        powerFactorObj["name"]    = "Power Factor";
        powerFactorObj["value"]   = sensor_data->powerFactor;
        powerFactorObj["unit"]    = "%";
    }
    

    // Temperature
    JsonObject temperatureObj = json_root["temperature"].to<JsonObject>();
    temperatureObj["name"]    = "Temperature";
    temperatureObj["value"]   = sensor_data->temperature;
    temperatureObj["unit"]    = "°C";

    String json_string;
    serializeJson(json_doc, json_string);

    if(json_string != "")
    {
        mqtt_payload = json_string;
        return true;
    }
    else return false;
    
}