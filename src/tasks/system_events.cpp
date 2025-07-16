/* Includes ------------------------------------------------------------------*/
#include "system_events.h"

/* Define --------------------------------------------------------------------*/

/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
QueueHandle_t               _system_cmd_queue = NULL;
app_system_state_t          _system_current_state;
static const char*          TAG                = "SYSTEM SUPERVISOR";

/* Functions -----------------------------------------------------------------*/
void init_system_supervisor(void)
{
    APP_LOGI(TAG, "Init system supervisor.");

    _system_cmd_queue = xQueueCreate(15, sizeof(app_system_event_t));
    ASSERT_BOOL(_system_cmd_queue != NULL, TAG, "Create system message queue failed.");

    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_system_supervisor,
        "SYSTEM SUPERVISOR",
        1024 * 10,
        NULL,
        25,
        NULL,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create system supervisor failed!");
}

void task_system_supervisor(void *pvParameters)
{
    app_system_event_t evt;

    _system_current_state = STATE_AP_MODE;
    // init ap mode here -> need to design a init system behavior
    start_ap_mode();

    for(;;)
    {
        if (xQueueReceive(_system_cmd_queue, &evt, portMAX_DELAY) == pdPASS) 
        {
            APP_LOGI(TAG, "Received command, start processing.");
            if (evt.command == CMD_SWITCH_TO_NORMAL_MODE && _system_current_state == STATE_AP_MODE) 
            {
                APP_LOGI(TAG, "Received command to switch to NORMAL mode.");
                
                // 1. Ra hiệu và/hoặc hủy task của chế độ AP
                if (_ap_main_loop_handler_t != NULL) 
                {
                    vTaskDelete(_ap_main_loop_handler_t);
                    _ap_main_loop_handler_t = NULL; // Quan trọng: Đặt lại handle
                }

                if (_event_task_handler_t != NULL)
                {
                    vTaskDelete(_event_task_handler_t);
                    _event_task_handler_t = NULL; // Quan trọng: Đặt lại handle   
                }

                vTaskDelay(pdMS_TO_TICKS(500));

                APP_LOGI(TAG, "Starting NORMAL mode tasks...");

                // creater resource for normal mode


                _system_current_state = STATE_NORMAL_MODE;
            }
             else if (evt.command == CMD_SWITCH_TO_AP_MODE && _system_current_state == STATE_NORMAL_MODE)
            {
                APP_LOGI(TAG, "Received command to switch to AP mode.");

                // delete all normal mode task

                vTaskDelay(pdMS_TO_TICKS(500));

                APP_LOGI(TAG, "Starting AP mode tasks...");
                
                start_ap_mode();

                _system_current_state = STATE_AP_MODE;
            }
        }
       
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
