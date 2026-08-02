#ifndef MSG_CRC_H_
#define MSG_CRC_H_

#include <stdint.h> /* uint */

uint16_t msg_crc_compute(const uint8_t *data, uint16_t len);

#endif /* MSG_CRC_H_ */
