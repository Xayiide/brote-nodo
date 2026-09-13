#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTasCreate, vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <projdefs.h> /* pdMS_TO_TICKS */

#include "system_mgr.h"
#include "msg_api.h"
#include "net.h"
#include "wifi.h"
#include "led.h"

#define TAG "SYSMGR"

#define SYSTASK_PERIOD_MS 100

static void sysmgr_task(void *p);

void sysmgr_init(void)
{
	wifi_init_sta();
	led_init();

	msg_send_node_started();

	xTaskCreate(&sysmgr_task, "sysmgr_task", 4096, NULL, 1, NULL);
}


void sysmgr_task(void *p)
{
	for (;;) {
		led_main();
		vTaskDelay(pdMS_TO_TICKS(SYSTASK_PERIOD_MS));
	}
}
