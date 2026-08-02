#ifndef MSG_BUILDER_H_
#define MSG_BUILDER_H_

#include <stdint.h>

#include "msg_types.h"

enum msg_st msg_build_light_sample(struct msg_frame *frame,
                                   float lx,
                                   float wh);

#endif /* MSG_BUILDER_H_ */
