#ifndef BYTE_UTILS_H_
#define BYTE_UTILS_H_

#include <stdint.h> /* uint */

uint32_t bswap32(uint32_t n);
size_t   pack_le_32(uint8_t buf[4], uint32_t v);
size_t   pack_be_32(uint8_t buf[4], uint32_t v);
size_t   pack_be_16(uint8_t *buf, uint16_t v);
size_t   pack_be_8(uint8_t *buf, uint8_t v);
size_t   pack_be_float(uint8_t *buf, float v);

#endif /* BYTE_UTILS_H_ */
