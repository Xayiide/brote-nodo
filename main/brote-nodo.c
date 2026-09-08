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
	ESP_LOGI(TAG, "IP:DATA - %s:%d", DST_IP, DATA_PORT);
	ESP_LOGI(TAG, "IP:LOG  - %s:%d", DST_IP, LOG_PORT);

	wifi_init_sta();
	net_init(DST_IP, (uint16_t) DATA_PORT, (uint16_t) LOG_PORT);

	i2c_master_setup();

	snsmgr_init();

	return;
}

