#ifndef NET_H_
#define NET_H_

#include <stdint.h> /* uint */
#include <stddef.h> /* size_t */



void net_init(const char *ip, uint16_t data_port, uint16_t log_port);
void net_udp_send(const uint8_t *data, size_t len);
void net_log_send(const uint8_t *data, size_t len);
uint32_t bswap32(uint32_t n);
void     pack_le_32(uint8_t buf[4], uint32_t v);
void     pack_be_32(uint8_t buf[4], uint32_t v);

#endif /* NET_H_ */
