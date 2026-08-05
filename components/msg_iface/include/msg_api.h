#ifndef MSG_API_H_
#define MSG_API_H_

#include "msg_types.h"

enum msg_st msg_send_light_sample(float lx, float wh);
enum msg_st msg_send_hum_temp(float hum, float temp);

#endif /* MSG_API_H_ */
