#include <stdint.h> /* uint */
#include <stdio.h> /* TODO Borrar */


#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
//#include <freertos/event_groups.h>
#include <driver/i2c.h> /* i2c */


#include <esp_log.h> /* ESP_LOG */

#include "wifi.h"
#include "net.h"
#include "veml7700.h"
#include "msg_api.h"

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
	ESP_LOGI(TAG, "IP:DATA - %s:%d\n", DST_IP, DATA_PORT);
	ESP_LOGI(TAG, "IP:LOG  - %s:%d\n", DST_IP, LOG_PORT);

	wifi_init_sta();
	net_init(DST_IP, (uint16_t) DATA_PORT, (uint16_t) LOG_PORT);

	i2c_master_setup();
	veml7700_init();
	veml7700_add_dev(0x10, 1000);

	float lx, wh;
	int lx_int, lx_dec, wh_int, wh_dec;

	while (1) {
		lx = veml7700_lux(0x10);
		wh = veml7700_white(0x10);

		lx_int = (int) lx;
		lx_dec = (int) ((lx - lx_int) * 10000);
		if (lx_dec < 0)
			lx_dec = -lx_dec; /* Evitar decimales negativos */

		wh_int = (int) wh;
		wh_dec = (int) ((wh - wh_int) * 10000);
		if (wh_dec < 0)
			wh_dec = -wh_dec;

		printf("Lux:   %d.%04d lx\n", lx_int, lx_dec);
		printf("White: %d.%04d lx\n", wh_int, wh_dec);

		msg_send_light_sample(lx, wh);

		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	return;

}

