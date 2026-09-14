#include <string.h> /* memcpy */
#include <lwip/inet.h> /* INET_ADDRSTRLEN */

#include <sdkconfig.h>

#include "network_mgr.h"
#include "net.h"

void netmgr_init(void)
{
	struct net_params params;

	strncpy(params.ip, CONFIG_NODE_IP, INET_ADDRSTRLEN);
	params.ip[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.gw, CONFIG_NODE_GW, INET_ADDRSTRLEN);
	params.gw[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.submask, CONFIG_NODE_SUBMASK, INET_ADDRSTRLEN);
	params.submask[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(params.dst_ip, CONFIG_NODE_DST_IP, INET_ADDRSTRLEN);
	params.dst_ip[INET_ADDRSTRLEN - 1] = '\0';
	params.data_port   = (uint16_t) CONFIG_NODE_DATA_PORT;
	params.log_port    = (uint16_t) CONFIG_NODE_LOG_PORT;
	params.listen_port = (uint16_t) CONFIG_NODE_LISTEN_PORT;

	net_init(&params);
}
