#include <stdint.h> /* uint */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/i2c.h> /* i2c */
#include <portmacro.h> /* TickType_t */
#include <projdefs.h> /* pdMS_TO_TICKS */

#include <esp_log.h> /* ESP_LOG */
#include <sdkconfig.h>

#include "network_mgr.h"
#include "sensor_mgr.h"
#include "system_mgr.h"

#if !defined(CONFIG_NODE_IP)
#error "IP no está definido"
#endif

#if !defined(CONFIG_NODE_GW)
#error "GW no está definido"
#endif

#if !defined(CONFIG_NODE_SUBMASK)
#error "SUBMASK no está definido"
#endif

#if !defined(CONFIG_NODE_DST_IP)
#error "DST_IP no está definido"
#endif

#if !defined(CONFIG_NODE_DATA_PORT) || CONFIG_NODE_DATA_PORT == 0
#error "DATA_PORT no definido o inválido"
#endif

#if !defined(CONFIG_NODE_LOG_PORT) || CONFIG_NODE_LOG_PORT == 0
#error "LOG_PORT no definido o inválido"
#endif

#if !defined(CONFIG_NODE_LISTEN_PORT) || CONFIG_NODE_LISTEN_PORT == 0
#error "LISTEN_PORT no definido o inválido"
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
	ESP_LOGI(TAG, "IP:DATA - %s:%d", CONFIG_NODE_DST_IP,
	                                 CONFIG_NODE_DATA_PORT);
	ESP_LOGI(TAG, "IP:LOG  - %s:%d", CONFIG_NODE_DST_IP,
	                                 CONFIG_NODE_LOG_PORT);

	i2c_master_setup();

	netmgr_init();
	sysmgr_init();
	snsmgr_init();

	return;
}

