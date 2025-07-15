#include "web_manager_services.h"


AsyncWebServer          g_http_server(80);
WebSocketsServer        g_ws_server(81);

static QueueHandle_t    g_event_queue               = NULL;

static char             _ap_ssid_temp[32]          = "";
static uint8_t          g_is_ap_logged_in           = 0;
static TickType_t       g_last_activity_tick        = 0;
static TickType_t       g_login_timestamp_tick      = 0;
char                    _ap_ssid[65]                = AP_SSID_DEFAULT;
char                    _ap_password[65]            = AP_PASSWORD_DEFAULT;

char                    _id[11]                     = ID_DEFAULT;            
uint16_t                _version                    = VERSION_DEFAULT;

static const char*      TAG                         = "WEB MANAGER";



void start_ap_mode()
{
    APP_LOGI(TAG, "Ap mode started.");

    g_event_queue = xQueueCreate(10, sizeof(app_event_t));
    ASSERT_BOOL(g_event_queue != NULL, TAG, "Failed to create event queue");

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_event_handler,
        "EVENT HANDLER TASK",
        1024 * 10,
        NULL,
        5,
        NULL,
        1
    );
    ASSERT_BOOL(task_result == pdPASS, TAG, "Failed to create EVENT HANDLER TASK");

    // ASSERT_BOOL(WiFi.disconnect(), TAG, "Failed to disconnect WiFi");
    // ASSERT_BOOL(WiFi.mode(WIFI_OFF), TAG, "Failed to turn off WiFi");

    snprintf(_ap_ssid, sizeof(_ap_ssid), "ABC_SMH_SMARTLUG_%s", _id);
    sprintf(_ap_ssid_temp, "%s %s", DEVICE_NAME, _id);

    ASSERT_BOOL(WiFi.mode(WIFI_AP_STA), TAG, "Failed to set WiFi to AP+STA mode");
    ASSERT_BOOL(WiFi.softAP(_ap_ssid, _ap_password), TAG, "Failed to start softAP");

    APP_LOGI(TAG, "AP Started");
    APP_LOGI(TAG, "AP SSID: %s", _ap_ssid);
    APP_LOGI(TAG, "AP Password: %s", _ap_password);

    IPAddress IP = WiFi.softAPIP();
    APP_LOGI(TAG, "AP IP: %s", IP.toString().c_str());

    setup_http_server_endpoints();
    g_http_server.begin();  // Không có return bool, giả định sẽ không lỗi nghiêm trọng
    g_ws_server.onEvent(websocket_event_callback);
    g_ws_server.begin();    // Tương tự, không có return

    BaseType_t ws_task_result = xTaskCreatePinnedToCore(
        task_ap_main_loop,
        "WEBSOCKET LOOP HANDLER",
        1024 * 10,
        NULL,
        1,
        NULL,
        1
    );
    ASSERT_BOOL(ws_task_result == pdPASS, TAG, "Failed to create WEBSOCKET LOOP HANDLER task");

    APP_LOGI(TAG, "AP Mode with unified event handler is running.");
}


/**
 * @brief Đăng ký tất cả các endpoint cho HTTP Web Server. 
 */
void setup_http_server_endpoints() {

    // --- CÁC ENDPOINT ĐƠN GIẢN: CHỈ TRẢ VỀ DỮ LIỆU/TRANG TĨNH ---
    // Các endpoint này rất nhanh, không cần đưa vào queue xử lý.

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
        if (g_is_ap_logged_in) { // Giả sử ap_logged_in là biến global được quản lý an toàn
            request->send(200, "text/html", html_admin);
        } else {
            request->send(200, "text/html", html_login);
        }
    });

    g_http_server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = String(WIFI_SSID_DEFAULT) + "," + String(_ap_ssid_temp);
        request->send(200, "text/plain", data.c_str());
    });
    
    g_http_server.on("/scan_wifi", HTTP_GET, [](AsyncWebServerRequest *request){
        // Logic quét WiFi bất đồng bộ, trả về ngay lập tức để không block.
        // Đây là một tác vụ đặc biệt, không cần queue nếu thư viện đã hỗ trợ non-blocking.
        if(WiFi.scanComplete() == WIFI_SCAN_RUNNING){
            request->send(503, "text/plain", "Scan in progress");
            return;
        }
        WiFi.scanNetworks(true);
        request->send(200, "text/plain", "Scan started");
    });


    // --- CÁC ENDPOINT PHỨC TẠP: CẦN XỬ LÝ LOGIC, GHI DỮ LIỆU ---
    // Áp dụng mô hình "Nhận, Giao việc, Phản hồi".

    g_http_server.on("/updateWiFi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("new_ssid", true) && request->hasParam("new_password", true)) {
            app_event_t evt;
            evt.source = EVT_SRC_HTTP_SERVER;
            evt.type   = HTTP_REQ_UPDATE_WIFI;
            
            strncpy(evt.data.http.param1, request->getParam("new_ssid", true)->value().c_str(), sizeof(evt.data.http.param1) - 1);
            strncpy(evt.data.http.param2, request->getParam("new_password", true)->value().c_str(), sizeof(evt.data.http.param2) - 1);

            if (xQueueSend(g_event_queue, &evt, 0) == pdPASS) {
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
            
            if (xQueueSend(g_event_queue, &evt, 0) == pdPASS) {
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
        
        if (xQueueSend(g_event_queue, &evt, 0) == pdPASS) {
            request->send(200, "text/plain", "OK. Switching to normal mode...");
        } else {
            request->send(503, "text/plain", "Server busy.");
        }
    });

    // Xử lý các trang không tồn tại
    g_http_server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}


/**
 * @brief Hàm callback của WebSocket Server. Chỉ đóng gói và gửi sự kiện đi.
 * @note  Hàm này thay thế hoàn toàn phiên bản cũ.
 */
void websocket_event_callback(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    // Chỉ xử lý nếu queue đã được khởi tạo
    if (g_event_queue == NULL) {
        return;
    }

    app_event_t evt;
    evt.source = EVT_SRC_WEBSOCKET; // Đánh dấu nguồn là WebSocket

    switch (type) {
        case WStype_DISCONNECTED:
        {
            evt.type = WEBSOCKET_DISCONNECTED;
            evt.data.ws.client_num = num;
            xQueueSend(g_event_queue, &evt, 0);
            break;
        }
        case WStype_CONNECTED:
        {
            evt.type = WEBSOCKET_CONNECTED;
            evt.data.ws.client_num = num;
            xQueueSend(g_event_queue, &evt, 0);
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
            
            xQueueSend(g_event_queue, &evt, 0);
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
        if(xQueueReceive(g_event_queue, &evt, portMAX_DELAY) == pdPASS)
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
                    // log
                    // save wifi
                    break;
                
                case HTTP_REQ_UPDATE_ADMIN:                    
                    APP_LOGI(TAG, "Processing 'Update Admin' request.");

                    // if (g_is_ap_logged_in) {
                    //     // save_admin_credentials(evt.data.http.param1, evt.data.http.param2);
                    // } else {
                    //     APP_LOGI(TAG, "Denied. Not logged in.");                        
                    // }
                    break;
                
                case HTTP_REQ_CHECK_LOGIN:
                    APP_LOGI(TAG, "Processing 'Check login' request.");

                    // if (strcmp(evt.data.http.param1, AP_ADMIN_USERNAME) == 0 && strcmp(evt.data.http.param2, AP_ADMIN_PASSWORD) == 0) {
                    //     g_is_ap_logged_in = true;
                    //     // Lưu lại thời điểm đăng nhập bằng tick
                    //     g_login_timestamp_tick = xTaskGetTickCount(); 
                    // } else {
                    //     g_is_ap_logged_in = false;
                    // }

                    break;
                
                case HTTP_REQ_GO_NORMAL:
                    APP_LOGI(TAG, "Processing 'Go to Normal Mode' request.");
                    // switch_to_normal_mode();

                    break;
                
                default:

                    break;

                }
                break;
            
            case EVT_SRC_WEBSOCKET:
                switch (evt.type)
                {
                case WEBSOCKET_CONNECTED:
                    APP_LOGI(TAG, "Processing 'Websocket connected' request.");
                    
                    break;
                
                case WEBSOCKET_DISCONNECTED:
                    APP_LOGI(TAG, "Processing 'Websocket disconnected' request.");

                    break;
                
                case WEBSOCKET_TEXT_RECEIVED:
                    APP_LOGI(TAG, "Processing 'Websocket text received' request.");

                    break;
                
                default:

                    break;

                }

                break;

            default:
                break;
            }
        }
        else
        {
            TickType_t current_tick = xTaskGetTickCount();

            // 1. Check & processing timeout event for AP mode
            if ((current_tick - g_last_activity_tick) > pdMS_TO_TICKS(AP_TIMEOUT_MS)) {
                APP_LOGI(TAG, "AP mode timed out. Switching to normal mode.");                
                app_event_t self_evt;

                // if we dont have any event, system will go back to normal mode
                self_evt.type = HTTP_REQ_GO_NORMAL;
                xQueueSend(g_event_queue, &self_evt, 0);
            }

            // 2. Check & processing timeout event for login session
            if (g_is_ap_logged_in && ((current_tick - g_login_timestamp_tick) > pdMS_TO_TICKS(LOGIN_TIMEOUT_MS))) {
                APP_LOGI(TAG, "Login session timed out.");                                
                g_is_ap_logged_in = false;
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