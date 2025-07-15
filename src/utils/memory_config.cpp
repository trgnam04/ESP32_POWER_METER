/* Includes ------------------------------------------------------------------*/
#include "memory_config.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
uint8_t          _wifi_ssid_length                       = strlen(WIFI_SSID_DEFAULT);
uint8_t          _wifi_password_length                   = strlen(WIFI_PASSWORD_DEFAULT);
char             _wifi_ssid[33]                          = WIFI_SSID_DEFAULT;
char             _wifi_password[65]                      = WIFI_PASSWORD_DEFAULT;
char             _ap_ssid[65]                            = AP_SSID_DEFAULT;
char             _ap_password[65]                        = AP_PASSWORD_DEFAULT;

char             _mqtt_host[65]                          = MQTT_HOST;
char             _mqtt_username[65]                      = MQTT_USERNAME;
char             _mqtt_password[65]                      = MQTT_PASSWORD;
char             _mqtt_topic_sub[65]                     = MQTT_TOPIC_SUB;
char             _mqtt_topic_pub[65]                     = MQTT_TOPIC_PUB;
uint32_t         _mqtt_port                              = MQTT_PORT;

uint16_t         _address_version                        = 0x0000;
uint16_t         _address_id                             = 0x0000;
uint16_t         _address_reset_count                    = 0x0000;

uint16_t         _address_station_code                   = 0x0000;

uint16_t         _address_wifi_ssid                      = 0x0000;
uint16_t         _address_wifi_password                  = 0x0000;

uint16_t         _address_ap_ssid                        = 0x0000;
uint16_t         _address_ap_password                    = 0x0000;


uint16_t         _address_mqtt_host                      = 0x0000;
uint16_t         _address_mqtt_username                  = 0x0000;
uint16_t         _address_mqtt_password                  = 0x0000;
uint16_t         _address_mqtt_topic_sub                 = 0x0000;
uint16_t         _address_mqtt_topic_pub                 = 0x0000;
uint16_t         _address_mqtt_port                      = 0x0000;

uint16_t         _address_bl0940_RMSOS                   = 0x0000;
uint16_t         _address_bl0940_WATTOS                  = 0x0000;
uint16_t         _address_bl0940_WA_CREEP                = 0x0000;

uint16_t         _address_bl0940_R_DIVIDE_CALIB          = 0x0000;
uint16_t         _address_bl0940_RL_CALIB                = 0x0000;
uint16_t         _address_bl0940_kWh                     = 0x0000;

static uint32_t  bl0940_RMSOS                            = 0x00000000;
static uint32_t  bl0940_WATTOS                           = 0x00000000;
static uint32_t  bl0940_WA_CREEP                         = 0x00000000;
static uint16_t  reset_count                             = 0;
static float     bl0940_R_DIVIDE_CALIB                   = 1.0;
static float     bl0940_RL_CALIB                         = 1.0;

float bl0940_kWh = 0.0;

/* Functions -----------------------------------------------------------------*/
void set_address(uint16_t* startAddr, uint16_t* sourceAddr, uint16_t numOfAddr, uint16_t sizeOfPara)
{
  uint8_t _i;
  uint16_t _addr = *startAddr;

  for (_i = 0; _i < numOfAddr; _i++)
  {
    *(sourceAddr + _i) = _addr;
    _addr              = _addr + sizeOfPara;
  }
  *startAddr = _addr;
}



// Initialization & update
void memory_init(void)
{
    uint16_t _addr = 0x0000;

  // device setting
  set_address(&_addr, &_address_version, 1, sizeof(_version));
  set_address(&_addr, &_address_id, 1, sizeof(_id));
  set_address(&_addr, &_address_reset_count, 1, sizeof(reset_count));
  set_address(&_addr, &_address_station_code, 1, sizeof(_station_code));

  // wifi setting
  set_address(&_addr, &_address_wifi_ssid, 1, sizeof(_wifi_ssid));
  set_address(&_addr, &_address_wifi_password, 1, sizeof(_wifi_password));

  // mqtt setting
  set_address(&_addr, &_address_mqtt_host, 1, sizeof(_mqtt_host));
  set_address(&_addr, &_address_mqtt_username, 1, sizeof(_mqtt_username));
  set_address(&_addr, &_address_mqtt_password, 1, sizeof(_mqtt_password));
  set_address(&_addr, &_address_mqtt_topic_sub, 1, sizeof(_mqtt_topic_sub));
  set_address(&_addr, &_address_mqtt_topic_pub, 1, sizeof(_mqtt_topic_pub));
  set_address(&_addr, &_address_mqtt_port, 1, sizeof(_mqtt_port));

  // bl0940 setting
  set_address(&_addr, &_address_bl0940_RMSOS, 1, sizeof(bl0940_RMSOS));
  set_address(&_addr, &_address_bl0940_WATTOS, 1, sizeof(bl0940_WATTOS));
  set_address(&_addr, &_address_bl0940_WA_CREEP, 1, sizeof(bl0940_WA_CREEP));
  set_address(&_addr, &_address_bl0940_R_DIVIDE_CALIB, 1, sizeof(bl0940_R_DIVIDE_CALIB));
  set_address(&_addr, &_address_bl0940_RL_CALIB, 1, sizeof(bl0940_RL_CALIB));
  set_address(&_addr, &_address_bl0940_kWh, 1, sizeof(bl0940_kWh));    
}

void memory_update_version(void)
{
  EepromWrite16b(_address_version, _version);
}

// WiFi
void memory_save_wifi_config(void)
{
  EepromWriteString(_address_wifi_ssid, _wifi_ssid, sizeof(_wifi_ssid));
  EepromWriteString(_address_wifi_password, _wifi_password, sizeof(_wifi_password));
}

void memory_load_wifi_config(void)
{
  char read_wifi_ssid[32];
  char read_wifi_password[64];
  EepromReadString(_address_wifi_ssid, read_wifi_ssid, 33);
  EepromReadString(_address_wifi_password, read_wifi_password, 65);

  strcpy(_wifi_ssid, read_wifi_ssid);
  strcpy(_wifi_password, read_wifi_password);
  _wifi_ssid_length     = strlen(_wifi_ssid);
  _wifi_password_length = strlen(_wifi_password);

}

// MQTT
void memory_save_mqtt_config(void)
{
  EepromWriteString(_address_mqtt_host, _mqtt_host, sizeof(_mqtt_host));
  EepromWriteString(_address_mqtt_username, _mqtt_username, sizeof(_mqtt_username));
  EepromWriteString(_address_mqtt_password, _mqtt_password, sizeof(_mqtt_password));
  EepromWriteString(_address_mqtt_topic_sub, _mqtt_topic_sub, sizeof(_mqtt_topic_sub));
  EepromWriteString(_address_mqtt_topic_pub, _mqtt_topic_pub, sizeof(_mqtt_topic_pub));
  EepromWrite32b(_address_mqtt_port, _mqtt_port);
}

void memory_load_mqtt_config(void)
{
  char read_mqtt_host[65];
  char read_mqtt_username[65];
  char read_mqtt_password[65];
  char read_mqtt_topic_sub[65];
  char read_mqtt_topic_pub[65];

  EepromReadString(_address_mqtt_host, read_mqtt_host, 65);
  EepromReadString(_address_mqtt_username, read_mqtt_username, 65);
  EepromReadString(_address_mqtt_password, read_mqtt_password, 65);
  EepromReadString(_address_mqtt_topic_sub, read_mqtt_topic_sub, 65);
  EepromReadString(_address_mqtt_topic_pub, read_mqtt_topic_pub, 65);

  strcpy(_mqtt_host, read_mqtt_host);
  strcpy(_mqtt_username, read_mqtt_username);
  strcpy(_mqtt_password, read_mqtt_password);
  strcpy(_mqtt_topic_sub, read_mqtt_topic_sub);
  strcpy(_mqtt_topic_pub, read_mqtt_topic_pub);
  _mqtt_port = EepromRead32b(_address_mqtt_port);

}

// Access Point
void memory_save_ap_config(void)
{
  EepromWriteString(_address_ap_password, _ap_password, sizeof(_ap_password));
}

void memory_load_ap_config(void)
{
  char read_ap_password[65];
  EepromReadString(_address_ap_password, read_ap_password, 65);

  strcpy(_ap_password, read_ap_password);

}

// Device ID & Station Code
void memory_save_device_id(void)
{ 
  EepromWriteString(_address_id, _id, 11);
}

void memory_load_device_id(void)
{
  char read_id[11];
  EepromReadString(_address_id, read_id, 11);

  strcpy(_id, read_id);
}

void memory_save_station_code(void)
{
  EepromWriteString(_address_station_code, _station_code, sizeof(_station_code));
}

void memory_load_station_code(void)
{
  char read_station_code[17];
  EepromReadString(_address_station_code, read_station_code, 17);

  strcpy(_station_code, read_station_code);
}

// Reset Count
void memory_save_reset_count(void)
{
  EepromWrite16b(_address_reset_count, reset_count);
}

void memory_load_reset_count(void)
{
  reset_count = EepromRead16b(_address_reset_count);
}

// BL0940 Sensor
void memory_save_bl0940_offset(void)
{
  EepromWrite32b(_address_bl0940_RMSOS, bl0940_RMSOS);
  EepromWrite32b(_address_bl0940_WATTOS, bl0940_WATTOS);
  EepromWrite32b(_address_bl0940_WA_CREEP, bl0940_WA_CREEP);
}

void memory_load_bl0940_offset(void)
{
  bl0940_RMSOS    = EepromRead32b(_address_bl0940_RMSOS);
  bl0940_WATTOS   = EepromRead32b(_address_bl0940_WATTOS);
  bl0940_WA_CREEP = EepromRead32b(_address_bl0940_WA_CREEP);
}

void memory_save_bl0940_calibration(void)
{
  EepromWrite32b(_address_bl0940_R_DIVIDE_CALIB, bl0940_R_DIVIDE_CALIB);
  EepromWrite32b(_address_bl0940_RL_CALIB, bl0940_RL_CALIB);
}

void memory_load_bl0940_calibration(void)
{
  bl0940_R_DIVIDE_CALIB = EepromRead32b(_address_bl0940_R_DIVIDE_CALIB);
  bl0940_RL_CALIB       = EepromRead32b(_address_bl0940_RL_CALIB);
}

void memory_save_bl0940_energy_kwh(void)
{
  EepromWrite32b(_address_bl0940_kWh, bl0940_kWh);
}

void memory_load_bl0940_energy_kwh(void)
{
  char read_station_code[17];
  EepromReadString(_address_station_code, read_station_code, 17);

  strcpy(_station_code, read_station_code);
}
