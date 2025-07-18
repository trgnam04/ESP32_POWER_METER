/* Includes ------------------------------------------------------------------*/
#include "input_processing.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
TaskHandle_t        _input_processing_handler_t    = NULL;
static const char*  TAG                            = "INPUT PROCESSING";

/* Functions -----------------------------------------------------------------*/
void init_input_processing(void)
{
    APP_LOGI(TAG, "Init input processing.");    
    button_init();

    BaseType_t task_result = xTaskCreatePinnedToCore(
      task_input_processing,
      "INPUT PROCESSING",
      1024 * 5,
      NULL,
      1,      
      &_input_processing_handler_t,
      1
    );

    ASSERT_BOOL(task_result, TAG, "Create input processing task failed.");
}

void task_input_processing(void *pvParameters)
{
    const TickType_t delay_ticks = pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS);    

    for(;;)
    {
        button_process();

        if (button_is_short_pressed())
        {
            APP_LOGI(TAG, "Button short pressed");
            APP_LOGI(TAG, "Toggle Relay");
            relay_toggle();
        }
        else if (button_is_long_pressed())
        {
            APP_LOGI(TAG, "Button long pressed. Requesting switch to AP mode");

            app_system_event_t cmd_evt;
            
            if (_system_current_state == STATE_NORMAL_MODE) {
                APP_LOGI(TAG, "System is in NORMAL mode. Requesting switch to AP mode.");
                cmd_evt.command = CMD_SWITCH_TO_AP_MODE;
            } else if (_system_current_state == STATE_AP_MODE)
            { 
                APP_LOGI(TAG, "System is in AP mode. Requesting switch to NORMAL mode.");
                cmd_evt.command = CMD_SWITCH_TO_NORMAL_MODE;
            }

            xQueueSend(_system_cmd_queue, &cmd_evt, 0);
            vTaskDelay(pdMS_TO_TICKS(3000)); 
        }

        vTaskDelay(delay_ticks);

    }

}