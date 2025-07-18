#include "common.h"
#include "web_manager_services.h"
#include "input_processing.h"
#include "system_events.h"



void setup() {  

  Serial.begin(9600);    
  delay(1000);
  init_system_supervisor();  

  // WiFi.begin("BKIT_CS2", "cselabc5c6");
  // while (WiFi.status() != WL_CONNECTED) {
  //     delay(500);
  //     Serial.print(".");
  // }
  // Serial.println("\nWiFi connected");

  // ntp_time_init();

}

void loop() {  
  // char buffer[32];
  // ntp_time_get_string(buffer, sizeof(buffer));
  // Serial.println(buffer);  
  // delay(1000);
}
