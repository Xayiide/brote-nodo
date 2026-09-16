#include <stdint.h> /* uint */
#include <stddef.h> /* NULL, size_t */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskCreate, vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <portmacro.h> /* TickType_t */
#include <sdkconfig.h>

#include "sensor_mgr.h"
#include "veml7700.h"
#include "am2315c.h"
#include "msg_types.h"
#include "msg_api.h"
#include "node_config.h"

#define TAG "SNSMGR"


struct snsmgr_cfg {
	const struct sensor_config *veml;
	const struct sensor_config *am23;
};

static struct snsmgr_cfg snsmgr = {
#if VEML7700_COUNT > 0
	.veml = veml7700_cfgs,
#endif
#if AM2315C_COUNT > 0
	.am23 = am2315c_cfgs,
#endif
};

static void snsmgr_task(void *p);
static void process_veml7700_dev(const struct sensor_config *dev);
static void process_am2315c_dev(const struct sensor_config *dev);

void snsmgr_init(void)
{
	int i;

	veml7700_init();
	am2315c_init();

	for (i = 0; i < VEML7700_COUNT; i++)
		veml7700_add_dev(snsmgr.veml[i].addr, snsmgr.veml[i].period_ms);

	for (i = 0; i < AM2315C_COUNT; i++)
		am2315c_add_dev(snsmgr.am23[i].addr, snsmgr.am23[i].period_ms);

	xTaskCreate(&snsmgr_task, "snsmgr_task", 4096, NULL, 1, NULL);
}

/* Funciones estáticas */

void snsmgr_task(void *p)
{
	TickType_t veml_wait, am23_wait, min_wait;
	int        i;

	for (;;) {
		veml_wait = veml7700_get_min_wait();
		am23_wait = am2315c_get_min_wait();

		min_wait = (veml_wait < am23_wait) ? veml_wait : am23_wait;

		if (min_wait > 0) {
			vTaskDelay(min_wait);
		}

		/* Volver a preguntar por si al despertar tras el vTaskDelay
		 * se ha pasado el temporizador de más de un sensor */
		if (veml7700_get_min_wait() == 0) {
			veml7700_poll();
			for (i = 0; i < VEML7700_COUNT; i++)
				if (veml7700_is_sample_ready(snsmgr.veml[i].addr))
					process_veml7700_dev(&(snsmgr.veml[i]));
		}

		if (am2315c_get_min_wait() == 0) {
			am2315c_poll();
			for (i = 0; i < AM2315C_COUNT; i++)
				if (am2315c_is_sample_ready(snsmgr.am23[i].addr))
					process_am2315c_dev(&(snsmgr.am23[i]));
		}
	}
}

void process_veml7700_dev(const struct sensor_config *dev)
{
	struct light_sample msg;
	esp_err_t error;

	error = veml7700_get_dev_err(dev->addr);

	if (error == ESP_OK)
		error = veml7700_get_lux(dev->addr, &msg.lx);

	if (error == ESP_OK)
		error = veml7700_get_white(dev->addr, &msg.wh);

	if (error == ESP_OK)
		error = veml7700_get_res(dev->addr, &msg.res);

	if (error == ESP_OK)
		error = veml7700_get_raw(dev->addr, &msg.raw_lx, &msg.raw_wh);

	if (error == ESP_OK)
		error = veml7700_get_cfg(dev->addr, &msg.it, &msg.gain);

	if (error == ESP_OK) {
		msg.sensor_id = dev->id;
		msg_send_light_sample(&msg);
		ESP_LOGD(TAG, "VEML7700: Muestra enviada");
	} else {
		ESP_LOGE(TAG, "VEML7700 0x%02X: %s",
			dev->addr, esp_err_to_name(error));
	}
}

void process_am2315c_dev(const struct sensor_config *dev)
{
	struct hum_temp_sample msg;
	esp_err_t error;

	error = am2315c_get_dev_err(dev->addr);

	if (error == ESP_OK)
		error = am2315c_get_hum(dev->addr, &msg.hum);

	if (error == ESP_OK)
		error = am2315c_get_temp(dev->addr, &msg.temp);

	if (error == ESP_OK) {
		msg.sensor_id = dev->id;
		msg_send_hum_temp(&msg);
		ESP_LOGD(TAG, "AM2315C: Muestra enviada");
	} else {
		ESP_LOGE(TAG, "AM2315C 0x%02X: %s",
			dev->addr, esp_err_to_name(error));
	}
}
