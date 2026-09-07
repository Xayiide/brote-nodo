#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskCreate, VTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <portmacro.h> /* TickType_t */

#include "sensor_mgr.h"

#define TAG "SNSMGR"

static void main_task(void *p);

void snsmgr_init(void)
{
	veml7700_init();
	am2315c_init();

	veml7700_add_dev(0x10, 1000);
	am2315c_add_dev(0x38, 2000);

	xTaskCreate(&main_task, "snsmgr_task", 4096, NULL, 1, NULL);
}


void main_task(void *p)
{
	TickType_t veml_wait, am_wait, min_wait;

	for (;;) {
		veml_wait = veml7700_get_min_wait();
		am_wait = am2315c_get_min_wait();

		min_wait = (veml_wait < am_wait) ? veml_wait : am_wait;

		if (min_wait > 0)
			vTaskDelay(min_wait);

		/* Volver a preguntar por si al despertar tras el vTasDelay
		 * se ha pasado el temporizador de más de un sensor */
		if (veml7700_get_min_wait() == 0)
			veml7700_read_all_devs();

		if (am2315c_get_min_wait() == 0)
			am2315c_read_all_devs();
	}
}
