#ifndef __COMMON_H_
#define __COMMON_H_


/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>
#include <ArduinoJson.h>

#include <EEPROM.h>

#include <WebSocketsServer.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <MQTT.h>
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


/* Define --------------------------------------------------------------------*/

// information
#define PRODUCT_NAME            "Smart Lug"
#define PRODUCT_MODEL           "SL.00.00.01"
#define FIRMWARE_VERSION        "1"
#define BUILD_NUMBER            "D202405.0001"

// device setting
#define STATION_CODE            "SL"
#define DEVICE_NAME             "Smart Lug"
#define ID_DEFAULT              "0000"
#define VERSION_DEFAULT         1

// wifi setting
#define WIFI_SSID_DEFAULT       "BKIT_CS2"
#define WIFI_PASSWORD_DEFAULT   "cselabc5c6"

// mqtt setting
#define MQTT_HOST               "mqtt.abcsolutions.com.vn"
#define MQTT_USERNAME           "abcsolution"
#define MQTT_PASSWORD           "CseLAbC5c6"
#define MQTT_TOPIC_SUB          "/smartLug/sub"
#define MQTT_TOPIC_PUB          "/smartLug/pub"
#define MQTT_PORT               1883

// ap setting
#define AP_SSID_DEFAULT         "AP-ESP8266"
#define AP_PASSWORD_DEFAULT     "12345678"
#define AP_ADMIN_USERNAME       "admin"
#define AP_ADMIN_PASSWORD       "adminisme"

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

// task manager
extern TaskHandle_t     _event_task_handler_t;
extern TaskHandle_t     _ap_main_loop_handler_t;

#endif // __COMMON_H