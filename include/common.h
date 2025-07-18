#ifndef __COMMON_H_
#define __COMMON_H_


/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <ArduinoJson.h>

#include <EEPROM.h>
#include <SPI.h>

#include <WebSocketsServer.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <MQTT.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncOTA.h>

#include <string.h>
#include <cstring>
#include <WString.h>
#include <stdint.h>
#include <deque>

#include "app_log.h"
#include "app_assert.h"
#include "bl0940.h"


/* Define --------------------------------------------------------------------*/

// information
#define PRODUCT_NAME                "Smart Plug"
#define PRODUCT_MODEL               "SL.00.00.01"
#define FIRMWARE_VERSION            "1"
#define BUILD_NUMBER                "D202405.0001"

// device setting
#define STATION_CODE                "SL"
#define DEVICE_NAME                 "Smart Plug"
#define ID_DEFAULT                  "0001"
#define VERSION_DEFAULT             1

// wifi setting
#define WIFI_SSID_DEFAULT           "BKIT_CS2"
#define WIFI_PASSWORD_DEFAULT       "cselabc5c6"

// mqtt setting
#define NTP_SERVER                  "pool.ntp.org"
#define MQTT_HOST                   "mqtt.abcsolutions.com.vn"
#define MQTT_USERNAME               "abcsolution"
#define MQTT_PASSWORD               "CseLAbC5c6"
#define MQTT_TOPIC_SUB              "/smartPlug/sub"
#define MQTT_TOPIC_PUB              "/smartPlug/pub"
#define MQTT_PORT                   1883

// ap setting
#define AP_SSID_DEFAULT             "AP-ESP32"
#define AP_PASSWORD_DEFAULT         "12345678"
#define AP_ADMIN_USERNAME           "admin"
#define AP_ADMIN_PASSWORD           "adminisme"

#define WIFI_CONNECTED_BIT          (1 << 0)
#define MQTT_CONNECTED_BIT          (1 << 1)
#define DATA_FROM_SENSOR_BIT        (1 << 2)
#define NTP_SYNCED_BIT              (1 << 3)

/* Struct --------------------------------------------------------------------*/
typedef enum 
{
    CMD_SWITCH_TO_AP_MODE,
    CMD_SWITCH_TO_NORMAL_MODE
} app_system_command_t;

typedef enum
{
    STATE_AP_MODE,
    STATE_NORMAL_MODE
} app_system_state_t;

typedef struct
{
    app_system_command_t command;
} app_system_event_t;

typedef struct {
    char topic[64];
    char payload[256];    
} mqtt_message_t;

typedef struct
{
    float voltage;
    float current;
    float activePower;
    float activeEnergy;
    float powerFactor;
    float temperature;
    bool is_valid;
} raw_sensor_data_t;

// Web manager
// Định danh nguồn của sự kiện
typedef enum {
    EVT_SRC_HTTP_SERVER,
    EVT_SRC_WEBSOCKET,
    EVT_SRC_SENSOR
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
    WEBSOCKET_TEXT_RECEIVED,

    // Sự kiện từ sensor processing
    SENSOR_DATA_READY
} event_type_t;

// Cấu trúc dữ liệu cho từng loại sự kiện
typedef struct {
    char param1[64];
    char param2[64];
} http_request_data_t;

typedef struct {
    uint8_t client_num;
    char    payload[128];
} websocket_data_t;

typedef struct {
    char payload[256];
} sensor_data_t;

// Cấu trúc sự kiện hợp nhất - "Phong bì" chung
typedef struct {
    event_source_t source; // Nguồn gốc sự kiện
    event_type_t   type;   // Loại sự kiện là gì

    union {
        http_request_data_t http;
        websocket_data_t    ws;
        sensor_data_t sensor;
    } data; // Dữ liệu đi kèm, dùng chung vùng nhớ
} app_event_t;

typedef enum {
    FSM_WS_IDLE,
    FSM_WS_WAITING_FOR_WIFI,
    FSM_WS_WAITING_FOR_ACK,
    FSM_WS_FINISHED
} ws_fsm_state_t;


/* Variables -----------------------------------------------------------------*/
extern char             _id[11];
extern uint16_t         _version;
extern char             _station_code[17];

extern uint8_t          _wifi_ssid_length;
extern uint8_t          _wifi_password_length;
extern char             _wifi_ssid[33];
extern char             _wifi_password[65];
extern char             _ap_ssid[65];
extern char             _ap_password[65];

extern char             _mqtt_host[65];
extern char             _mqtt_username[65];
extern char             _mqtt_password[65];
extern char             _mqtt_topic_sub[65];
extern char             _mqtt_topic_pub[65];
extern uint32_t         _mqtt_port;

extern uint16_t         _address_version;
extern uint16_t         _address_id;
extern uint16_t         _address_reset_count;

extern uint16_t         _address_station_code;

extern uint16_t         _address_wifi_ssid;
extern uint16_t         _address_wifi_password;

extern uint16_t         _address_ap_ssid;
extern uint16_t         _address_ap_password;

extern uint16_t         _address_mqtt_host;
extern uint16_t         _address_mqtt_username;
extern uint16_t         _address_mqtt_password;
extern uint16_t         _address_mqtt_topic_sub;
extern uint16_t         _address_mqtt_topic_pub;
extern uint16_t         _address_mqtt_port;

extern uint16_t         _address_bl0940_RMSOS;
extern uint16_t         _address_bl0940_WATTOS;
extern uint16_t         _address_bl0940_WA_CREEP;

extern uint16_t         _address_bl0940_R_DIVIDE_CALIB;
extern uint16_t         _address_bl0940_RL_CALIB;
extern uint16_t         _address_bl0940_kWh;

extern uint32_t         bl0940_RMSOS;
extern uint32_t         bl0940_WATTOS;                           
extern uint32_t         bl0940_WA_CREEP;                         
extern uint16_t         reset_count;
extern float            bl0940_kWh;

extern float            bl0940_R_DIVIDE_CALIB;                   
extern float            bl0940_RL_CALIB;


/* System Manager ------------------------------------------------------------*/
// task manager
extern TaskHandle_t             _event_task_handler_t;
extern TaskHandle_t             _ap_main_loop_handler_t;
extern TaskHandle_t             _input_processing_handler_t;
extern TaskHandle_t             _mqtt_client_handler_t;
extern TaskHandle_t             _wifi_manager_handler_t;
extern TaskHandle_t             _app_logic_handler_t;
extern TaskHandle_t             _sensor_processing_handler_t;
extern TaskHandle_t             _ntp_sync_handler_t;


// queue manager
// system command queue
extern QueueHandle_t            _system_cmd_queue;

// normal mode communication data queue
extern QueueHandle_t            _mqtt_outgoing_queue;
extern QueueHandle_t            _mqtt_incoming_queue;
extern QueueHandle_t            _raw_sensor_data_queue;
extern QueueHandle_t            _web_manager_event_queue;

// system state
extern app_system_state_t       _system_current_state;


// Event group
// normal mode event group
extern EventGroupHandle_t       _normal_mode_event_group;


#endif // __COMMON_H