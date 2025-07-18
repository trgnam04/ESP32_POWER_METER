/* Includes ------------------------------------------------------------------*/
#include "sensor_processing.h"

/* Define --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/
static const char*              TAG                             = "SENSOR PROCESSING";
// this object has construction func
BL0940                          bl0940;
TaskHandle_t                    _sensor_processing_handler_t    = NULL;

/* Functions -----------------------------------------------------------------*/
void init_sensor_processing(void)
{
    APP_LOGI(TAG, "Init sensor processing task.");
    
    BaseType_t task_result = xTaskCreatePinnedToCore(
        task_sensor_processing,
        "SENSOR PROCESSING",
        1024 * 5,
        NULL,
        1,
        &_sensor_processing_handler_t,
        1
    );

    ASSERT_BOOL(task_result, TAG, "Create sensor processing task failed.");
}

// file: sensor_processing.cpp

void task_sensor_processing(void* pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t period = SENSOR_READ_PERIOD; // Ví dụ: 1000ms
    static uint32_t warning_timeslot = 0;

    for(;;)
    {
        /** Update needed
         *  We need to modify this bl0940 library
         * 1. Define error code for some certain functions.
         * 2. Apply system log for all function.
         * 3. Fix some save-to-memory function.
         * 
         * Fix this get-sensor-data flow control later
         * 1. Define error log base on reach over the warning timeslot time threshold
         * 2. Cause this task can run on many state so we will change it to FSM later with some explicit state.
         */
        raw_sensor_data_t sensor_reading = {0}; // Khởi tạo sạch sẽ

        // Cố gắng đọc dữ liệu từ cảm biến
        if (bl0940.getData())
        {
            sensor_reading.is_valid     = true;
            sensor_reading.voltage      = bl0940.voltage;
            sensor_reading.current      = bl0940.current;
            sensor_reading.activePower  = bl0940.activePower;
            sensor_reading.activeEnergy = bl0940.activeEnergy;
            sensor_reading.powerFactor  = bl0940.powerFactor;
            sensor_reading.temperature  = bl0940.temperature;
            
            xQueueSend(_raw_sensor_data_queue, &sensor_reading, 0);
        }
        else
        {
            sensor_reading.is_valid = false;
            warning_timeslot++;
            APP_LOGW(TAG, "%d time: Sensor doesn't response in this timeslot.", warning_timeslot);
        }

        if(warning_timeslot == ERROR_THRESHOLD)
        {
            APP_LOGE(TAG, "Something wrong with your sensor, please check the data connection");
            break;
        }
        
        // Gửi dữ liệu (dù thành công hay thất bại) vào hàng đợi trung gian
        // để task logic có thể xử lý (ví dụ: báo lỗi nếu không hợp lệ)
        // xQueueSend(_raw_sensor_data_queue, &sensor_reading, 0);

        vTaskDelayUntil(&xLastWakeTime, period);
    }

    APP_LOGI(TAG, "Self deleting, check the data connection and restart system.");

    _sensor_processing_handler_t = NULL;

    vTaskDelete(NULL);
}
