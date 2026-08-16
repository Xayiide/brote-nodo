#ifndef CONFIG_H_
#define CONFIG_H_

#if defined(NODE_TERRAZA)
	#define CFG_NODE_ID            1
	#define CFG_SENSOR_ID_LIGHT    1
	#define CFG_SENSOR_ID_TEMP_HUM 2
#elif defined(NODE_INTERIOR)
	#define CFG_NODE_ID            2
	#define CFG_SENSOR_ID_LIGHT    1
	#define CFG_SENSOR_ID_TEMP_HUM 2
#else
	#error "NO NODE CONFIG DEFINED"
#endif

#endif /* CONFIG_H_ */
