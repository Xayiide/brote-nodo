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
	frame->hdr.id = 1; // TODO: rellenar el ID
	frame->hdr.src_id = 2; // TODO: rellenar el SRC_ID
	//frame->hdr.timestamp = 0;
	frame->hdr.body_len = off;
	frame->hdr.version = 3; // TODO: rellenar la VERSION

exit:
	return st;
}

