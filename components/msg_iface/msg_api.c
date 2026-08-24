#include <stdint.h>

#include "msg_api.h"
#include "msg_types.h"
#include "msg_builder.h"
#include "msg_serializer.h"

#include "net.h"

enum msg_st msg_send_light_sample(struct light_sample *sample)
{
	enum msg_st      st = MSG_OK;
	uint8_t          msg[MSG_MAX_FRAME_LEN];
	struct msg_frame frame;
	int32_t frame_len;

	st = msg_build_light_sample(&frame, sample);
	if (st != MSG_OK)
		goto exit;

	frame_len = msg_serialize(&frame, msg, sizeof(msg));
	if (frame_len <= 0) {
		st = MSG_ERR;
		goto exit;
	}

	net_udp_send(msg, frame_len);

exit:
	return st;
}

enum msg_st msg_send_hum_temp(float hum, float temp)
{
	enum msg_st      st;
	uint8_t          msg[MSG_MAX_FRAME_LEN];
	struct msg_frame frame;
	uint32_t         frame_len;

	st = msg_build_hum_temp(&frame, hum, temp);
	if (st != MSG_OK)
		goto exit;

	frame_len = msg_serialize(&frame, msg, sizeof(msg));
	if (frame_len <= 0) {
		st = MSG_ERR;
		goto exit;
	}

	net_udp_send(msg, frame_len);

exit:
	return st;
}
