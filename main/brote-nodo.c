#include <stdint.h> /* uint */
#include <stdio.h> /* TODO Borrar */


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h> /* i2c */


#include <esp_log.h> /* ESP_LOG */

#include "wifi.h"
#include "net.h"
#include "msg_types.h"
#include "msg_api.h"
#include "sensor_mgr.h"

#if !defined(DST_IP)
#error "DST_IP no está definido"
#endif

#if !defined(DATA_PORT) || DATA_PORT == 0
#error "DATA_PORT no definido o inválido"
#endif

#if !defined(LOG_PORT) || LOG_PORT == 0
#error "LOG_PORT no definido o inválido"
#endif

#define TAG "BROTE-NODO"

void float_to_int_dec(float v, int *integer, int *decimal)
{
	*integer = (int) v;
	*decimal = (int) ((v - *integer) * 10000);

	if (*decimal < 0)
		*decimal = -(*decimal);
}


void i2c_master_setup(void)
{
	i2c_config_t conf;

	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = GPIO_NUM_4;
	conf.scl_io_num = GPIO_NUM_5;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	// conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
	// conf.clk_flags = 0;

	i2c_driver_install(I2C_NUM_0, conf.mode);
	i2c_param_config(I2C_NUM_0, &conf);
}

void app_main(void)
{
	struct light_sample veml_sample;
	struct hum_temp_sample am23_sample;

	ESP_LOGI(TAG, "IP:DATA - %s:%d\n", DST_IP, DATA_PORT);
	ESP_LOGI(TAG, "IP:LOG  - %s:%d\n", DST_IP, LOG_PORT);

	wifi_init_sta();
	net_init(DST_IP, (uint16_t) DATA_PORT, (uint16_t) LOG_PORT);

	i2c_master_setup();

	snsmgr_init();

	uint8_t ret;

	int lx_int, lx_dec;
	int wh_int, wh_dec;
	int hum_int, hum_dec;
	int temp_int, temp_dec;
	int res_int, res_dec;

	while (1) {
		veml7700_get_lux(0x10, &veml_sample.lx);
		veml7700_get_white(0x10, &veml_sample.wh);

		ret = veml7700_get_res(0x10, &veml_sample.res);
		if (ret != 0)
			continue;

		ret = veml7700_get_raw(0x10, &veml_sample.raw_lx, &veml_sample.raw_wh);
		if (ret != 0)
			continue;

		ret = veml7700_get_cfg(0x10, &veml_sample.it, &veml_sample.gain);
		if (ret != 0)
			continue;

		am2315c_get_hum(0x38, &am23_sample.hum);
		am2315c_get_temp(0x38, &am23_sample.temp);

		float_to_int_dec(veml_sample.lx, &lx_int, &lx_dec);
		float_to_int_dec(veml_sample.wh, &wh_int, &wh_dec);
		float_to_int_dec(veml_sample.res, &res_int, &res_dec);
		float_to_int_dec(am23_sample.hum, &hum_int, &hum_dec);
		float_to_int_dec(am23_sample.temp, &temp_int, &temp_dec);

		if (veml_sample.wh != 0.0 && veml_sample.lx != 0.0) {
			//msg_send_light_sample(&veml_sample);
			printf("VEML7700: [res: %d.%04d] [it: %u] [gain: %u] | [lx raw: %u] [wh raw: %u]\n",
					res_int,
					res_dec,
					(unsigned int) veml_sample.it,
					(unsigned int) veml_sample.gain,
					(unsigned int) veml_sample.raw_lx,
					(unsigned int) veml_sample.raw_wh);
			printf("          [%d.%04d lx] [%d.%04d wh]\n",
					lx_int, lx_dec, wh_int, wh_dec);
		}

		if (am23_sample.hum != 0.0 && am23_sample.temp != 0.0) {
			//msg_send_hum_temp(hum, temp);
			printf("AM2108C : ");
			printf("[%d.%04d %% hum] [%d.%04d ºC]\n",
					hum_int, hum_dec, temp_int, temp_dec);
		}

		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	return;

}

