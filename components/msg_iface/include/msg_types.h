#ifndef MSG_TYPES_H_
#define MSG_TYPES_H_

#include <stdint.h> /* uint */

#define MSG_MAX_BODY_LEN 255
#define MSG_MAX_FRAME_LEN (sizeof(struct msg_hdr) + MSG_MAX_BODY_LEN)

enum msg_type {
	MSG_TYPE_LIGHT_SAMPLE,
	MSG_TYPE_HUM_TEMP,
	MSG_TYPE_LOG,
};

enum msg_st {
	MSG_OK,
	MSG_ERR,
	MSG_ERR_NULL_PARAM,
	MSG_ERR_CRC_MISMATCH,
	MSG_ERR_UNKNOWN_TYPE,
};

struct msg_hdr {
	uint8_t  type;
	uint8_t  syn;
	uint16_t src_id;
	//uint32_t timestamp;
	uint16_t body_len;
	uint16_t version;
} __attribute__((packed));

struct msg_frame {
	struct msg_hdr hdr;
	uint8_t body[MSG_MAX_BODY_LEN];
};

#endif /* MSG_TYPES_H_ */
