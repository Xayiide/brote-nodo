#ifndef MSG_BUILDER_H_
#define MSG_BUILDER_H_

#include <stdint.h>

#include "msg_types.h"

enum msg_st msg_build_light_sample(struct msg_frame *frame,
                                   struct light_sample *sample);
enum msg_st msg_build_hum_temp_sample(struct msg_frame *frame,
                                      struct hum_temp_sample *sample);
enum msg_st msg_build_node_started(struct msg_frame *frame);
enum msg_st msg_build_sensor_config(struct msg_frame *frame,
                                    struct sensor_group *sensors,
                                    uint8_t num);

#endif /* MSG_BUILDER_H_ */
