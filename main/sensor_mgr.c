#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskCreate, vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <portmacro.h> /* TickType_t */

#include "sensor_mgr.h"
#include "veml7700.h"
#include "am2315c.h"
#include "msg_types.h"
#include "msg_api.h"

#define TAG "SNSMGR"

#define VEML7700_ADDR_0 0x10
#define VEML7700_PERIOD_MS 2000

#define AM2315C_ADDR_0 0x38
#define AM2315C_PERIOD_MS 2000

struct veml_mgr {
	uint8_t addrs[VEML7700_MAX_NUM_DEV];
	uint8_t num;
};

struct am23_mgr {
	uint8_t addrs[AM2315C_MAX_NUM_DEV];
	uint8_t num;
};

struct snsmgr_cfg {
	struct veml_mgr veml;
	struct am23_mgr am23;
};

static struct snsmgr_cfg snsmgr = {
	.veml = {
		.addrs = {
			VEML7700_ADDR_0,
		},
		.num = 1,
	},
	.am23 = {
		.addrs = {
			AM2315C_ADDR_0,
		},
		.num = 1,
	},
};

static void snsmgr_task(void *p);
static void process_veml7700_dev(uint8_t addr);
static void process_am2315c_dev(uint8_t addr);

void snsmgr_init(void)
{
	uint8_t i;

	veml7700_init();
	am2315c_init();

	for (i = 0; i < snsmgr.veml.num; i++)
		veml7700_add_dev(snsmgr.veml.addrs[i], VEML7700_PERIOD_MS);

	for (i = 0; i < snsmgr.am23.num; i++)
		am2315c_add_dev(snsmgr.am23.addrs[i], AM2315C_PERIOD_MS);

	xTaskCreate(&snsmgr_task, "snsmgr_task", 4096, NULL, 1, NULL);
}

/* Funciones estáticas */

void snsmgr_task(void *p)
{
	TickType_t veml_wait, am23_wait, min_wait;
	uint8_t    i;

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
			for (i = 0; i < snsmgr.veml.num; i++)
				if (veml7700_is_sample_ready(snsmgr.veml.addrs[i]))
					process_veml7700_dev(snsmgr.veml.addrs[i]);
		}

		if (am2315c_get_min_wait() == 0) {
			am2315c_poll();
			for (i = 0; i < snsmgr.am23.num; i++)
				if (am2315c_is_sample_ready(snsmgr.am23.addrs[i]))
					process_am2315c_dev(snsmgr.am23.addrs[i]);
		}
	}
}

void process_veml7700_dev(uint8_t addr)
{
	struct light_sample msg;
	esp_err_t error;

	error = veml7700_get_dev_err(addr);

	if (error == ESP_OK)
		error = veml7700_get_lux(addr, &msg.lx);

	if (error == ESP_OK)
		error = veml7700_get_white(addr, &msg.wh);

	if (error == ESP_OK)
		error = veml7700_get_res(addr, &msg.res);

	if (error == ESP_OK)
		error = veml7700_get_raw(addr, &msg.raw_lx, &msg.raw_wh);

	if (error == ESP_OK)
		error = veml7700_get_cfg(addr, &msg.it, &msg.gain);

	if (error == ESP_OK) {
		msg_send_light_sample(&msg);
		ESP_LOGD(TAG, "VEML7700: Muestra enviada");
	} else {
		ESP_LOGE(TAG, "VEML7700 0x%02X: %s", addr, esp_err_to_name(error));
	}
}

void process_am2315c_dev(uint8_t addr)
{
	struct hum_temp_sample msg;
	esp_err_t error;

	error = am2315c_get_dev_err(addr);

	if (error == ESP_OK)
		error = am2315c_get_hum(addr, &msg.hum);

	if (error == ESP_OK)
		error = am2315c_get_temp(addr, &msg.temp);

	if (error == ESP_OK) {
		msg_send_hum_temp(&msg);
		ESP_LOGD(TAG, "AM2315C: Muestra enviada");
	} else {
		ESP_LOGE(TAG, "AM2315C 0x%02X: %s", addr, esp_err_to_name(error));
	}
}
