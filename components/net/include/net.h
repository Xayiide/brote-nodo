#ifndef NET_H_
#define NET_H_

#include <stdint.h> /* uint */
#include <stddef.h> /* size_t */

void net_init(const char *ip, uint16_t data_port, uint16_t log_port);
void net_udp_send(const uint8_t *data, size_t len);
void net_log_send(const uint8_t *data, size_t len);

#endif /* NET_H_ */
