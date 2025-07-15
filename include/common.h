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


#endif // __COMMON_H