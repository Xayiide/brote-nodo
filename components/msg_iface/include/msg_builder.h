#ifndef MSG_BUILDER_H_
#define MSG_BUILDER_H_

#include <stdint.h>

#include "msg_types.h"

enum msg_st msg_build_light_sample(struct msg_frame *frame,
                                   struct light_sample *sample);
enum msg_st msg_build_hum_temp(struct msg_frame *frame,
                               float hum,
                               float temp);

#endif /* MSG_BUILDER_H_ */
