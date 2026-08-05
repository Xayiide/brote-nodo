#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */
#include <string.h> /* memcpy */

#include "msg_serializer.h"
#include "msg_types.h"
#include "byte_utils.h"

int32_t msg_serialize(struct msg_frame *frame, uint8_t *body, uint16_t len)
{
	int32_t ret = 0;
	enum msg_st st = MSG_OK;
	size_t off = 0;
	uint16_t needed;

	needed = sizeof(frame->hdr) + frame->hdr.body_len;

	if (frame == NULL || body == NULL || len < needed) {
		st = MSG_ERR_NULL_PARAM;
		goto exit;
	}

	body[off] = frame->hdr.type;
	off += sizeof(frame->hdr.type);

	body[off] = frame->hdr.syn;
	off += sizeof(frame->hdr.syn);

	pack_be_16(body + off, frame->hdr.src_id);
	off += sizeof(frame->hdr.src_id);

	pack_be_16(body + off, frame->hdr.body_len);
	off += sizeof(frame->hdr.body_len);

	pack_be_16(body + off, frame->hdr.version);
	off += sizeof(frame->hdr.version);

	memcpy(body + off, frame->body, frame->hdr.body_len);
	off += frame->hdr.body_len;

	ret = (int32_t) off;

exit:
	if (st != MSG_OK)
		ret = -1;

	return ret;
}
