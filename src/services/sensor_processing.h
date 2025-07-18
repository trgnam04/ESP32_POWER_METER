/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "bl0940.h"
#include "app_log.h"
#include "app_assert.h"

/* Define --------------------------------------------------------------------*/
#define SENSOR_READ_PERIOD pdMS_TO_TICKS(1000)
#define ERROR_THRESHOLD                 200
/* Struct --------------------------------------------------------------------*/

/* Variables -----------------------------------------------------------------*/


/* Functions -----------------------------------------------------------------*/
void init_sensor_processing(void);
void task_sensor_processing(void* pvParameters);


/** NOTE
 * We push all data to one topic on the broker server, the data processing procedure below:
 * 1. Create a json object to save all data value that read from sensor.
 * 2. Get data from sensor and save it to buffer, note that we should solve the synchronization problem between tasks.
 * 3. Serialize it into string, this string data will be sent to server. The server will parse, classify data into corresponding field
 *  and display the data corretly base on the parser configuration.
 */