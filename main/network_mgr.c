#include <string.h> /* memcpy */
#include <lwip/inet.h> /* INET_ADDRSTRLEN */

#include "network_mgr.h"
#include "net.h"

void netmgr_init(void)
{
	struct net_params params;

	strncpy(params.ip, IP, INET_ADDRSTRLEN);
	params.ip[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.gw, GW, INET_ADDRSTRLEN);
	params.gw[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.submask, SUBMASK, INET_ADDRSTRLEN);
	params.submask[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.dst_ip, DST_IP, INET_ADDRSTRLEN);
	params.dst_ip[INET_ADDRSTRLEN - 1] = '\0';
	params.data_port   = (uint16_t) DATA_PORT;
	params.log_port    = (uint16_t) LOG_PORT;
	params.listen_port = (uint16_t) LISTEN_PORT;

	net_init(&params);
}
