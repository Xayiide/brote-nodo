#ifndef SENSOR_MGR_H_
#define SENSOR_MGR_H_

#include <sdkconfig.h>

struct sensor_cfg {
	uint16_t id;
	uint8_t  addr;
	uint32_t period_ms;
};

void snsmgr_init(void);

#define VEML7700_TYPENAME "VEML7700"
#define AM2315C_TYPENAME  "AM2315C"

#if CONFIG_AM2315C_0_ENABLE || CONFIG_AM2315C_1_ENABLE
static const struct sensor_cfg am2315c_cfgs[] = {
#if CONFIG_AM2315C_0_ENABLE
	{
		.id        = CONFIG_AM2315C_0_ID,
		.addr      = CONFIG_AM2315C_0_ADDR,
		.period_ms = CONFIG_AM2315C_0_PERIOD_MS
	},
#endif
#if CONFIG_AM2315C_1_ENABLE
	{
		.id        = CONFIG_AM2315C_1_ID,
		.addr      = CONFIG_AM2315C_1_ADDR,
		.period_ms = CONFIG_AM2315C_1_PERIOD_MS
	}
#endif
};
#endif
#define AM2315C_COUNT (CONFIG_AM2315C_0_ENABLE + CONFIG_AM2315C_1_ENABLE)

#if CONFIG_VEML7700_0_ENABLE || CONFIG_VEML7700_1_ENABLE
static const struct sensor_cfg veml7700_cfgs[] = {
#if CONFIG_VEML7700_0_ENABLE
	{
		.id        = CONFIG_VEML7700_0_ID,
		.addr      = CONFIG_VEML7700_0_ADDR,
		.period_ms = CONFIG_VEML7700_0_PERIOD_MS
	},
#endif
#if CONFIG_VEML7700_1_ENABLE
	{
		.id        = CONFIG_VEML7700_1_ID,
		.addr      = CONFIG_VEML7700_1_ADDR,
		.period_ms = CONFIG_VEML7700_1_PERIOD_MS
	}
#endif
};
#define VEML7700_COUNT (CONFIG_VEML7700_0_ENABLE + CONFIG_VEML7700_1_ENABLE)
#else
#define VEML7700_COUNT 0
#endif

#define SENSOR_COUNT (VEML7700_COUNT + AM2315C_COUNT)

#endif /* SENSOR_MGR_H_ */
