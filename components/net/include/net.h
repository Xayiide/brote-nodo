#ifndef NET_H_
#define NET_H_

#include <stdint.h> /* uint */
#include <stddef.h> /* size_t */
#include <lwip/inet.h> /* INET_ADDRSTRLEN */

struct net_params {
	char ip[INET_ADDRSTRLEN];
	char gw[INET_ADDRSTRLEN];
	char submask[INET_ADDRSTRLEN];
	char dst_ip[INET_ADDRSTRLEN];
	uint16_t    data_port;
	uint16_t    log_port;
	uint16_t    listen_port;
};

void net_init(struct net_params *params);
void net_udp_send(const uint8_t *data, size_t len);
void net_log_send(const uint8_t *data, size_t len);

#endif /* NET_H_ */
