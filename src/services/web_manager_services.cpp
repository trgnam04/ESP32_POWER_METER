#include "web_manager_services.h"


AsyncWebServer          g_http_server(80);
WebSocketsServer        g_ws_server(81);

static char             g_ap_ssid_temp[32]          = "";
static uint8_t          g_is_ap_logged_in           = 0;
static TickType_t       g_last_activity_tick        = 0;
static TickType_t       g_login_timestamp_tick      = 0;

char                    _id[11]                     = ID_DEFAULT;            
char                    _station_code[17]           = STATION_CODE;
TaskHandle_t            _event_task_handler_t       = NULL;
TaskHandle_t            _ap_main_loop_handler_t     = NULL;

char                    _ap_ssid[65]                = AP_SSID_DEFAULT;
char                    _ap_password[65]            = AP_PASSWORD_DEFAULT;

static const char*      TAG                         = "WEB MANAGER";

static ws_fsm_state_t   s_ws_fsm_state              = FSM_WS_IDLE;
static uint8_t          s_current_ws_client_num     = 0;
static TickType_t       s_ws_last_sent_tick         = 0; // Để xử lý timeout gửi lại

static char             s_latest_sensor_json[256]   = "";
static SemaphoreHandle_t s_sensor_data_mutex        = NULL;


static raw_sensor_data_t    sensor_data_local_buffer;


static void send_ws_json(const JsonDocument& doc) {
    String json_string;
    serializeJson(doc, json_string);
    g_ws_server.sendTXT(s_current_ws_client_num, json_string);
}


void start_ap_mode()
{
    APP_LOGI(TAG, "Ap mode started.");    

    s_sensor_data_mutex = xSemaphoreCreateMutex();
    ASSERT_BOOL(s_sensor_data_mutex != NULL, TAG, "Failed to create sensor data mutex");


    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_event_handler,
        "EVENT HANDLER TASK",
        1024 * 10,
        NULL,
        5,
        &_event_task_handler_t,
        1
    );
    ASSERT_BOOL(task_result == pdPASS, TAG, "Failed to create EVENT HANDLER TASK");

    // ASSERT_BOOL(WiFi.disconnect(), TAG, "Failed to disconnect WiFi");
    // ASSERT_BOOL(WiFi.mode(WIFI_OFF), TAG, "Failed to turn off WiFi");

    snprintf(_ap_ssid, sizeof(_ap_ssid), "ABC_SMH_SMARTLUG_%s", _id);
    sprintf(g_ap_ssid_temp, "%s %s", DEVICE_NAME, _id);

    ASSERT_BOOL(WiFi.mode(WIFI_AP_STA), TAG, "Failed to set WiFi to AP+STA mode");
    ASSERT_BOOL(WiFi.softAP(_ap_ssid, _ap_password), TAG, "Failed to start softAP");

    APP_LOGI(TAG, "AP Started");
    APP_LOGI(TAG, "AP SSID: %s", _ap_ssid);
    APP_LOGI(TAG, "AP Password: %s", _ap_password);

    IPAddress IP = WiFi.softAPIP();
    APP_LOGI(TAG, "AP IP: %s", IP.toString().c_str());

    setup_http_server_endpoints();
    g_http_server.begin();  
    delay(100);
    g_ws_server.onEvent(websocket_event_callback);
    g_ws_server.begin();    

    BaseType_t ws_task_result = xTaskCreatePinnedToCore(
        task_ap_main_loop,
        "WEBSOCKET LOOP HANDLER",
        1024 * 10,
        NULL,
        1,
        &_ap_main_loop_handler_t,
        1
    );
    ASSERT_BOOL(ws_task_result == pdPASS, TAG, "Failed to create WEBSOCKET LOOP HANDLER task");

    APP_LOGI(TAG, "AP Mode with unified event handler is running.");
}



void setup_http_server_endpoints() {
    

    g_http_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", html_monitor);
    });

    g_http_server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", html_setting);
    });

    g_http_server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", html_login);
    });

    g_http_server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (g_is_ap_logged_in) { 
            request->send(200, "text/html", html_admin);
        } else {
            request->send(200, "text/html", html_login);
        }
    });

    g_http_server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = String(_wifi_ssid) + "," + String(g_ap_ssid_temp);
        request->send(200, "text/plain", data.c_str());
    });

    g_http_server.on("/PowerMeter", HTTP_GET, [](AsyncWebServerRequest* request)
    {        
        raw_sensor_data_t sensor_copy; 

     
        if (xSemaphoreTake(s_sensor_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
     
     
            sensor_copy = sensor_data_local_buffer;
            
     
            xSemaphoreGive(s_sensor_data_mutex);
        }
        else
        {
     
            request->send(503, "text/plain", "Service temporarily unavailable");
            return;
        }

        String data_to_send = "";
        if (sensor_copy.is_valid) 
        {
        
            data_to_send += String(sensor_copy.voltage) + ",";
            data_to_send += String(sensor_copy.current) + ",";
            data_to_send += String(sensor_copy.activePower) + ",";
            data_to_send += String(sensor_copy.activeEnergy) + ",";
            
          
            if (sensor_copy.current == 0) {
                data_to_send += String(100) + ",";
            } else {
                data_to_send += String(sensor_copy.powerFactor) + ",";
            }

            data_to_send += String(sensor_copy.temperature);
        } 
        else 
        {

            data_to_send = "0,0,0,0,0,0"; 
        }
        
        request->send(200, "text/plain", data_to_send);
    });
    
    g_http_server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        if(WiFi.scanComplete() == WIFI_SCAN_RUNNING){
            request->send(503, "text/plain", "Scan in progress");
            return;
        }
        WiFi.scanNetworks(true);
        request->send(200, "text/plain", "Scan started");
    });



    g_http_server.on("/updateWiFi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("new_ssid", true) && request->hasParam("new_password", true)) {
            app_event_t evt;
            evt.source = EVT_SRC_HTTP_SERVER;
            evt.type   = HTTP_REQ_UPDATE_WIFI;
            
            strncpy(evt.data.http.param1, request->getParam("new_ssid", true)->value().c_str(), sizeof(evt.data.http.param1) - 1);
            strncpy(evt.data.http.param2, request->getParam("new_password", true)->value().c_str(), sizeof(evt.data.http.param2) - 1);

            evt.data.http.param1[sizeof(evt.data.http.param1) - 1] = '\0';
            evt.data.http.param2[sizeof(evt.data.http.param2) - 1] = '\0';

            if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
                request->send(200, "text/plain", "OK. WiFi update request received.");
            } else {
                request->send(503, "text/plain", "Server busy.");
            }
        } else {
            request->send(400, "text/plain", "Bad Request.");
        }
    });

    g_http_server.on("/updateAdmin", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("id", true) && request->hasParam("station_code", true)) {
            app_event_t evt;
            evt.source = EVT_SRC_HTTP_SERVER;
            evt.type   = HTTP_REQ_UPDATE_ADMIN;

            strncpy(evt.data.http.param1, request->getParam("id", true)->value().c_str(), sizeof(evt.data.http.param1) - 1);
            strncpy(evt.data.http.param2, request->getParam("station_code", true)->value().c_str(), sizeof(evt.data.http.param2) - 1);
            
            if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
                request->send(200, "text/plain", "OK. Admin update request received.");
            } else {
                request->send(503, "text/plain", "Server busy.");
            }
        } else {
            request->send(400, "text/plain", "Bad Request.");
        }
    });

    g_http_server.on("/checkLogin", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("username", true) && request->hasParam("password", true)) {
            app_event_t evt;
            evt.source = EVT_SRC_HTTP_SERVER;
            evt.type   = HTTP_REQ_CHECK_LOGIN;

            strncpy(evt.data.http.param1, request->getParam("username", true)->value().c_str(), sizeof(evt.data.http.param1) - 1);
            strncpy(evt.data.http.param2, request->getParam("password", true)->value().c_str(), sizeof(evt.data.http.param2) - 1);
            
            // Đối với login, bạn có thể xử lý nhanh và trả về kết quả ngay lập tức
            // thay vì đưa vào queue nếu việc kiểm tra chỉ là so sánh chuỗi.
            // Tuy nhiên, để nhất quán, ta vẫn có thể dùng queue.
            // Ở đây ví dụ xử lý trực tiếp để cho thấy sự linh hoạt.
            if (String(evt.data.http.param1) == AP_ADMIN_USERNAME && String(evt.data.http.param2) == AP_ADMIN_PASSWORD) {
                g_is_ap_logged_in = true;
                request->send(200, "text/plain", "Login successful");
            } else {
                request->send(401, "text/plain", "Login failed");
            }
        } else {
            request->send(400, "text/plain", "Bad Request.");
        }
    });

    g_http_server.on("/normal", HTTP_POST, [](AsyncWebServerRequest *request) {
        app_event_t evt;
        evt.source = EVT_SRC_HTTP_SERVER;
        evt.type   = HTTP_REQ_GO_NORMAL;
        
        if (xQueueSend(_web_manager_event_queue, &evt, 0) == pdPASS) {
            request->send(200, "text/plain", "OK. Switching to normal mode...");
        } else {
            request->send(503, "text/plain", "Server busy.");
        }
    });
    
    g_http_server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}


void websocket_event_callback(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {    
    if (_web_manager_event_queue == NULL) {
        return;
    }

    app_event_t evt;
    evt.source = EVT_SRC_WEBSOCKET;

    switch (type) {
        case WStype_DISCONNECTED:
        {
            evt.type = WEBSOCKET_DISCONNECTED;
            evt.data.ws.client_num = num;
            xQueueSend(_web_manager_event_queue, &evt, 0);
            break;
        }
        case WStype_CONNECTED:
        {
            evt.type = WEBSOCKET_CONNECTED;
            evt.data.ws.client_num = num;
            xQueueSend(_web_manager_event_queue, &evt, 0);
            break;
        }
        case WStype_TEXT:
        {
            evt.type = WEBSOCKET_TEXT_RECEIVED;
            evt.data.ws.client_num = num;
            
            // Sao chép payload một cách an toàn
            size_t len_to_copy = min(length, sizeof(evt.data.ws.payload) - 1);
            memcpy(evt.data.ws.payload, payload, len_to_copy);
            evt.data.ws.payload[len_to_copy] = '\0';
            
            xQueueSend(_web_manager_event_queue, &evt, 0);
            break;
        }                
        default:
        {
            break;
        }
            
    }
}

void task_event_handler(void *pvParameters)
{
    app_event_t evt;
    const TickType_t xBlockTime = pdMS_TO_TICKS(1000);

    g_last_activity_tick = xTaskGetTickCount();


    for(;;)
    {
        if(xQueueReceive(_web_manager_event_queue, &evt, portMAX_DELAY) == pdPASS)
        {
            g_last_activity_tick = xTaskGetTickCount();
            APP_LOGI(TAG, "Activity detected, AP timeout reset.");

            switch (evt.source)
            {
            case EVT_SRC_HTTP_SERVER:
                switch (evt.type)
                {
                case HTTP_REQ_UPDATE_WIFI:
                    APP_LOGI(TAG, "Processing 'Update Wifi' request.");
                    strcpy(_wifi_ssid ,evt.data.http.param1);
                    strcpy(_wifi_password, evt.data.http.param2);

                    _wifi_ssid_length = sizeof(_wifi_ssid);
                    _wifi_password_length = sizeof(_wifi_password_length);
                    
                    memory_save_wifi_config();                    
                    
                    break;
                
                case HTTP_REQ_UPDATE_ADMIN:                    
                    APP_LOGI(TAG, "Processing 'Update Admin' request.");

                    if (g_is_ap_logged_in) {
                        
                        strcpy(_id, evt.data.http.param1);
                        strcpy(_station_code, evt.data.http.param2);
                        memory_save_device_id();
                        memory_save_station_code();
                    } else {
                        APP_LOGW(TAG, "Denied. Not logged in.");                        
                    }
                    break;
                
                case HTTP_REQ_CHECK_LOGIN:
                    APP_LOGI(TAG, "Processing 'Check login' request.");

                    if (strcmp(evt.data.http.param1, AP_ADMIN_USERNAME) == 0 && strcmp(evt.data.http.param2, AP_ADMIN_PASSWORD) == 0) {
                        g_is_ap_logged_in = true;
                        // Lưu lại thời điểm đăng nhập bằng tick
                        g_login_timestamp_tick = xTaskGetTickCount(); 
                    } else {
                        g_is_ap_logged_in = false;
                    }

                    break;
                
                case HTTP_REQ_GO_NORMAL:
                    APP_LOGI(TAG, "Processing 'Go to Normal Mode' request.");
                    // delete every things related to web configuration & turn system into normal mode
                    // we need to turn off all mode related to ap host and websocket before we change to normal, cause
                    // those configurations are set downside the system (or we can think those configuration for taske 
                    // running on core 0 which for BLE and Wifi) so delete this task doesnt affect it.
                    app_system_event_t cmd_evt;
                    cmd_evt.command = CMD_SWITCH_TO_NORMAL_MODE;                    

                    xQueueSend(_system_cmd_queue, &cmd_evt, 0);
                                        
                    break;
                
                default:

                    break;

                }
                break;
            
            case EVT_SRC_WEBSOCKET:
                    switch (evt.type) {
                        case WEBSOCKET_CONNECTED:
                            s_current_ws_client_num = evt.data.ws.client_num;
                            APP_LOGI(TAG, "Handler: WS Client #%u connected.", s_current_ws_client_num);
                            
                            {
                                ArduinoJson::StaticJsonDocument<100> json_doc;
                                json_doc["state"] = "request_wifi";
                                send_ws_json(json_doc);
                                APP_LOGI(TAG, "Handler: Sent 'request_wifi'.");
                            }
                            s_ws_fsm_state = FSM_WS_WAITING_FOR_WIFI;
                            s_ws_last_sent_tick = xTaskGetTickCount(); 
                            break;

                        case WEBSOCKET_DISCONNECTED:
                            APP_LOGI(TAG, "Handler: WS Client #%u disconnected.", evt.data.ws.client_num);
                            s_ws_fsm_state = FSM_WS_IDLE; 
                            break;

                        case WEBSOCKET_TEXT_RECEIVED:
                            APP_LOGI(TAG, "Handler: WS msg from #%u: %s", evt.data.ws.client_num, evt.data.ws.payload);

                            ArduinoJson::StaticJsonDocument<256> json_doc;
                            if (deserializeJson(json_doc, evt.data.ws.payload) == DeserializationError::Ok) {
                                const char* state = json_doc["state"];
                                if (state) {
                                    // Xử lý dựa trên trạng thái FSM hiện tại
                                    if (strcmp(state, "response_wifi") == 0 && s_ws_fsm_state == FSM_WS_WAITING_FOR_WIFI) {
                                        const char* ssid = json_doc["ssid"];
                                        const char* password = json_doc["password"];
                                        memory_save_wifi_config();
                                        
                                        {
                                            ArduinoJson::StaticJsonDocument<128> out_json_doc;
                                            out_json_doc["state"] = "send_topic";
                                            out_json_doc["topic"] = "your_mqtt_topic"; 
                                            send_ws_json(out_json_doc);
                                            APP_LOGI(TAG, "Handler: Sent 'send_topic'.");
                                        }
                                        s_ws_fsm_state = FSM_WS_WAITING_FOR_ACK;
                                        s_ws_last_sent_tick = xTaskGetTickCount(); 
                                    } else if (strcmp(state, "receive_topic") == 0 && s_ws_fsm_state == FSM_WS_WAITING_FOR_ACK) {
                                        APP_LOGI(TAG, "Handler: WiFi config via WebSocket complete.");
                                        s_ws_fsm_state = FSM_WS_FINISHED;                                        
                                        app_event_t self_evt = { .source = EVT_SRC_HTTP_SERVER, .type = HTTP_REQ_GO_NORMAL };
                                        xQueueSend(_web_manager_event_queue, &self_evt, 0);
                                    }
                                }
                            }
                            break;
                    }
                    break;
                
                case EVT_SRC_SENSOR:
                {
                    switch(evt.type)
                    {
                        case SENSOR_DATA_READY:
                        {
                            APP_LOGI(TAG, "Handler: Received new sensor data, parsing and updating struct.");                            
                            StaticJsonDocument<256> json_doc;
                            DeserializationError error = deserializeJson(json_doc, evt.data.sensor.payload);

                            if (error) {
                                APP_LOGE(TAG, "Handler: Failed to parse sensor JSON: %s", error.c_str());
                                break; 
                            }
                            
                            if (xSemaphoreTake(s_sensor_data_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
                            {                                                                
                                sensor_data_local_buffer.voltage      = json_doc["voltage"];
                                sensor_data_local_buffer.current      = json_doc["current"];
                                sensor_data_local_buffer.activePower  = json_doc["activePower"];
                                sensor_data_local_buffer.activeEnergy = json_doc["activeEnergy"];
                                sensor_data_local_buffer.powerFactor  = json_doc["powerFactor"];
                                sensor_data_local_buffer.temperature  = json_doc["temperature"];
                                sensor_data_local_buffer.is_valid     = true;
                                                 
                                xSemaphoreGive(s_sensor_data_mutex);
                            }
                            else
                            {
                                APP_LOGW(TAG, "Handler: Could not get mutex to update sensor struct.");
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }
            }
        } else {
            // KHÔNG CÓ SỰ KIỆN MỚI -> KIỂM TRA CÁC LOẠI TIMEOUT
            TickType_t current_tick = xTaskGetTickCount();

            if ((current_tick - g_last_activity_tick) > pdMS_TO_TICKS(AP_TIMEOUT_MS)) {
                APP_LOGI(TAG, "AP mode timed out. Switching to normal mode.");
                app_event_t self_evt = { .source = EVT_SRC_HTTP_SERVER, .type = HTTP_REQ_GO_NORMAL };
                xQueueSend(_web_manager_event_queue, &self_evt, 0);
            }
            
            if (g_is_ap_logged_in && ((current_tick - g_login_timestamp_tick) > pdMS_TO_TICKS(LOGIN_TIMEOUT_MS))) {
                APP_LOGI(TAG, "Login session timed out.");
                g_is_ap_logged_in = false;
            }
            
            if (s_ws_fsm_state == FSM_WS_WAITING_FOR_WIFI && (current_tick - s_ws_last_sent_tick) > pdMS_TO_TICKS(3000)) {
                APP_LOGW(TAG, "Handler: Resending 'request_wifi' due to timeout.");
                ArduinoJson::StaticJsonDocument<100> json_doc;
                json_doc["state"] = "request_wifi";
                send_ws_json(json_doc);
                s_ws_last_sent_tick = current_tick; // Reset bộ đếm giờ
            } else if (s_ws_fsm_state == FSM_WS_WAITING_FOR_ACK && (current_tick - s_ws_last_sent_tick) > pdMS_TO_TICKS(3000)) {
                APP_LOGW(TAG, "Handler: Resending 'send_topic' due to timeout.");
                ArduinoJson::StaticJsonDocument<128> json_doc;
                json_doc["state"] = "send_topic";
                json_doc["topic"] = "your_mqtt_topic";
                send_ws_json(json_doc);
                s_ws_last_sent_tick = current_tick;
            }
        }

    }

}


void task_ap_main_loop(void *pvParameters)
{ 
    for(;;)
    {
        g_ws_server.loop(); // Required for websocket server
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}