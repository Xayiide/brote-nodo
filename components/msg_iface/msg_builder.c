#include <stddef.h> /* NULL */
#include <string.h> /* strlen, memcpy */

#include <sdkconfig.h>

#include "msg_builder.h"
#include "msg_types.h"
#include "byte_utils.h"


enum msg_st msg_build_light_sample(struct msg_frame *frame,
                                   struct light_sample *sample)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;

	if (frame == NULL || sample == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	off += pack_be_16(frame->body + off, sample->sensor_id);
	off += pack_be_float(frame->body + off, sample->lx);
	off += pack_be_float(frame->body + off, sample->wh);
	off += pack_be_float(frame->body + off, sample->res);
	off += pack_be_16(frame->body + off, sample->raw_lx);
	off += pack_be_16(frame->body + off, sample->raw_wh);
	off += pack_be_16(frame->body + off, sample->it);
	off += pack_be_8(frame->body + off, sample->gain);

	frame->hdr.msg_id = MSG_TYPE_LIGHT_SAMPLE;
	frame->hdr.syn = 0; /* TODO */
	frame->hdr.node_id = CONFIG_NODE_ID;
	frame->hdr.body_len = off;
	frame->hdr.version = 0;

exit:
	return st;
}

enum msg_st msg_build_hum_temp_sample(struct msg_frame *frame,
                                      struct hum_temp_sample *sample)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;

	if (frame == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	off += pack_be_16(frame->body + off, sample->sensor_id);
	off += pack_be_float(frame->body + off, sample->hum);
	off += pack_be_float(frame->body + off, sample->temp);

	frame->hdr.msg_id = MSG_TYPE_HUM_TEMP;
	frame->hdr.syn = 0;
	frame->hdr.node_id = CONFIG_NODE_ID;
	frame->hdr.body_len = off;
	frame->hdr.version = 0;

exit:
	return st;
}

enum msg_st msg_build_node_started(struct msg_frame *frame)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;

	if (frame == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	frame->hdr.msg_id = MSG_TYPE_NODE_START;
	frame->hdr.syn = 0;
	frame->hdr.node_id = CONFIG_NODE_ID;
	frame->hdr.body_len = off;
	frame->hdr.version = 0;

exit:
	return st;
}

enum msg_st msg_build_sensor_config(struct msg_frame *frame,
                                    struct sensor_group *sensors,
                                    uint8_t num)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;
	uint8_t i, j;
	size_t  type_len;

	if (frame == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	for (i = 0; i < num; i++) {
		/* Empaquetar nombre */
		type_len = strlen(sensors[i].type) + 1;
		if (type_len > MSG_MAX_TYPENAME_LEN) {
			st = MSG_ERR_TYPENAME_TOO_LONG;
			goto exit;
		}
		memcpy(frame->body + off, sensors[i].type, type_len);
		off += type_len;

		/* Empaquetar número */
		off += pack_be_8(frame->body + off, sensors[i].count);

		/* Para cada sensor, empaquetar id y periodo */
		for (j = 0; j < sensors[i].count; j++) {
			off += pack_be_16(frame->body + off, sensors[i].items[j].sensor_id);
			off += pack_be_32(frame->body + off, sensors[i].items[j].period_ms);
		}
	}

	frame->hdr.msg_id = MSG_TYPE_SENSOR_CONFIG;
	frame->hdr.syn = 0;
	frame->hdr.node_id = CONFIG_NODE_ID;
	frame->hdr.body_len = off;
	frame->hdr.version = 0;

exit:
	return st;
}
