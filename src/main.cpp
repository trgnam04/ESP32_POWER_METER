#include "common.h"
#include "web_manager_services.h"
#include "input_processing.h"
#include "system_events.h"



void setup() {
  Serial.begin(9600);
  memory_init();
  delay(100);
  init_system_supervisor();
  delay(100);
  init_input_processing();
  delay(100);    

}

void loop() {  
}
