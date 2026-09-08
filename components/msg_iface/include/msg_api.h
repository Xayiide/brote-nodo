#ifndef MSG_API_H_
#define MSG_API_H_

#include "msg_types.h"

enum msg_st msg_send_light_sample(struct light_sample *sample);
enum msg_st msg_send_hum_temp(struct hum_temp_sample *sample);

#endif /* MSG_API_H_ */
