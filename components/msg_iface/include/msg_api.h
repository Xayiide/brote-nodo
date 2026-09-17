#ifndef MSG_API_H_
#define MSG_API_H_

#include "msg_types.h"

enum msg_st msg_send_light_sample(struct light_sample *sample);
enum msg_st msg_send_hum_temp(struct hum_temp_sample *sample);
enum msg_st msg_send_node_started(void);
enum msg_st msg_send_sensor_config(struct sensor_group *data, uint8_t num);

#endif /* MSG_API_H_ */
