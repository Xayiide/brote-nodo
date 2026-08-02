#include <stdint.h> /* uint */
#include <string.h> /* memcpy */

#include "byte_utils.h"

inline uint32_t bswap32(uint32_t n)
{
	return ((n & 0xFF000000U) >> 24)
	     | ((n & 0x00FF0000U) >> 8)
	     | ((n & 0x0000FF00U) << 8)
	     | ((n & 0x000000FFU) << 24);
}

inline void pack_le_32(uint8_t buf[4], uint32_t v)
{
	memcpy(buf, &v, sizeof(v));
}

inline void pack_be_32(uint8_t buf[4], uint32_t v)
{
	v = bswap32(v);
	memcpy(buf, &v, sizeof(v));
}

inline void pack_be_16(uint8_t *buf, uint16_t v)
{
	buf[0] = (uint8_t) (v >> 8);
	buf[1] = (uint8_t) (v & 0xFF);
}

inline void pack_be_float(uint8_t *buf, float v)
{
	uint32_t bits;

	memcpy(&bits, &v, sizeof(bits));
	pack_be_32(buf, bits);
}




