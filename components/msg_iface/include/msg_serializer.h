#ifndef MSG_SERIALIZER_H_
#define MSG_SERIALIZER_H_

#include <stdint.h> /* uint */

#include "msg_types.h"

int32_t msg_serialize(struct msg_frame *frame, uint8_t *body, uint16_t len);

#endif /* MSG_SERIALIZER_H_ */
