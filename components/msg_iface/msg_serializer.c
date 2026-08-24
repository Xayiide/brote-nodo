#include <stdint.h> /* uint */
#include <stddef.h> /* NULL */
#include <string.h> /* memcpy */

#include "msg_serializer.h"
#include "msg_types.h"
#include "byte_utils.h"

int32_t msg_serialize(struct msg_frame *frame, uint8_t *body, uint16_t len)
{
	int32_t ret = -1;
	size_t off = 0;
	uint16_t needed;


	if (frame == NULL || body == NULL) {
		goto exit;
	}

	needed = sizeof(frame->hdr) + frame->hdr.body_len;

	if (len < needed) {
		goto exit;
	}

	off += pack_be_8(body + off, frame->hdr.msg_id);
	off += pack_be_8(body + off, frame->hdr.syn);
	off += pack_be_16(body + off, frame->hdr.node_id);
	off += pack_be_16(body + off, frame->hdr.sensor_id);
	off += pack_be_16(body + off, frame->hdr.body_len);
	off += pack_be_16(body + off, frame->hdr.version);

	memcpy(body + off, frame->body, frame->hdr.body_len);
	off += frame->hdr.body_len;

	ret = (int32_t) off;

exit:

	return ret;
}
