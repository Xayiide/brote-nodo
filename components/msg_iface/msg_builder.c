#include <stddef.h> /* NULL */

#include "msg_builder.h"
#include "msg_types.h"
#include "byte_utils.h"


enum msg_st msg_build_light_sample(struct msg_frame *frame,
                                   float lx,
                                   float wh)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;

	if (frame == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	pack_be_float(frame->body + off, lx);
	off += sizeof(float);
	pack_be_float(frame->body + off, wh);
	off += sizeof(float);

	frame->hdr.type = MSG_TYPE_LIGHT_SAMPLE;
	frame->hdr.syn = 1; // TODO: rellenar el ID
	frame->hdr.src_id = 2; // TODO: rellenar el SRC_ID
	//frame->hdr.timestamp = 0;
	frame->hdr.body_len = off;
	frame->hdr.version = 3; // TODO: rellenar la VERSION

exit:
	return st;
}

enum msg_st msg_build_hum_temp(struct msg_frame *frame,
                               float hum,
                               float temp)
{
	enum msg_st st = MSG_OK;
	size_t off = 0;

	if (frame == NULL) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	pack_be_float(frame->body + off, hum);
	off += sizeof(float);
	pack_be_float(frame->body + off, temp);
	off += sizeof(float);

	frame->hdr.type = MSG_TYPE_HUM_TEMP;
	frame->hdr.syn = 2; // TODO: rellenar el ID
	frame->hdr.src_id = 2; // TODO: rellenar el SRC_ID
	//frame->hdr.timestamp = 0;
	frame->hdr.body_len = off;
	frame->hdr.version = 3; // TODO: rellenar la VERSION

exit:
	return st;
}
