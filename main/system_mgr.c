#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTasCreate, vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <projdefs.h> /* pdMS_TO_TICKS */

#include "system_mgr.h"
#include "wifi.h"

#define SYSTASK_PERIOD_MS 100

static void sysmgr_task(void *p);

void sysmgr_init(void)
{
	xTaskCreate(&sysmgr_task, "sysmgr_task", 4096, NULL, 1, NULL);
}


void sysmgr_task(void *p)
{
	wifi_init_sta();


	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(SYSTASK_PERIOD_MS));
	}
}
