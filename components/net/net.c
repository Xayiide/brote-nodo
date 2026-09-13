#include <stdio.h>  /* sscanf */
#include <stdint.h> /* uint */
#include <string.h> /* strncpy */

#include <sys/socket.h> /* sockaddr_in, AF_INET, socket, sendto */
#include <netinet/in.h> /* INET_ADDRSTRLEN */
#include <lwip/def.h> /* htons */
#include <lwip/inet.h> /* inet_addr */
#include <tcpip_adapter.h> /* tcpip_adapter_init, dhcpc_stop */

#include <esp_log.h> /* ESP_LOG */

#include "net.h"

#define TAG "NET"

struct net_host {
	struct sockaddr_in addr;
	int socket;
};

struct net_cfg {
	struct net_params params;
	struct net_host data;
	struct net_host log;
	struct net_host listen;
};

static struct net_cfg net;

static void set_static_ip(char *ip, char *gw, char *submaks);
static inline bool init_params_are_valid(struct net_params *params);
static void init_cfg(struct net_cfg *cfg, struct net_params *params);
static void init_data_dest(struct net_host *data, struct net_params *params);
static void init_log_dest(struct net_host *log, struct net_params *params);
static void init_listen(struct net_host *listen, struct net_params *params);

void net_init(struct net_params *params)
{
	if (init_params_are_valid(params) == false) {
		ESP_LOGE(TAG, "IP or ports invalid");
		return;
	}

	init_cfg(&net, params);
	tcpip_adapter_init();
	set_static_ip(params->ip, params->gw, params->submask);
	init_data_dest(&net.data, params);
	init_log_dest(&net.log, params);
	init_listen(&net.listen, params);

	/* xTaskCreate ... */
}

void net_udp_send(const uint8_t *data, size_t len)
{
	sendto(net.data.socket,
	       data,
	       len,
	       0,
	       (struct sockaddr *) &net.data.addr,
	       sizeof(net.data.addr));
}

void net_log_send(const uint8_t *data, size_t len)
{
	sendto(net.log.socket,
	       data,
	       len,
	       0,
	       (struct sockaddr *) &net.log.addr,
	       sizeof(net.log.addr));
}

/* Funciones estáticas */

void set_static_ip(char *ip, char *gw, char *submask)
{
	tcpip_adapter_ip_info_t ip_info;
	unsigned int a, b, c, d;
	const char *ip_fmt = "%u.%u.%u.%u";

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

	if (sscanf(ip, ip_fmt, &a, &b, &c, &d) != 4) {
		ESP_LOGE(TAG, "Error scanning ip: %s", ip);
		goto exit;
	}
	IP4_ADDR(&ip_info.ip, 172, 20, 10, 200);

	if (sscanf(gw, ip_fmt, &a, &b, &c, &d) != 4) {
		ESP_LOGE(TAG, "Error scanning gw: %s", gw);
		goto exit;
	}
	IP4_ADDR(&ip_info.gw, 172, 20, 10, 1);

	if (sscanf(gw, ip_fmt, &a, &b, &c, &d) != 4) {
		ESP_LOGE(TAG, "Error scanning submask: %s", submask);
		goto exit;
	}
	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);

exit:
	return;
}

inline bool init_params_are_valid(struct net_params *params)
{
	return ((params->ip != NULL)
	     && (params->dst_ip != NULL)
	     && (params->data_port != 0)
	     && (params->log_port != 0)
	     && (params->listen_port != 0));
}

void init_cfg(struct net_cfg *cfg, struct net_params *params)
{
	strncpy(cfg->params.ip, params->ip, INET_ADDRSTRLEN);
	cfg->params.ip[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(cfg->params.gw, params->gw, INET_ADDRSTRLEN);
	cfg->params.gw[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(cfg->params.submask, params->submask, INET_ADDRSTRLEN);
	cfg->params.submask[INET_ADDRSTRLEN - 1] = '\0';
	strncpy(cfg->params.dst_ip, params->dst_ip, INET_ADDRSTRLEN);
	cfg->params.dst_ip[INET_ADDRSTRLEN - 1] = '\0';

	cfg->params.data_port   = params->data_port;
	cfg->params.log_port    = params->log_port;
	cfg->params.listen_port = params->listen_port;
}

static void init_data_dest(struct net_host *data, struct net_params *params)
{
	data->addr.sin_family      = AF_INET;
	data->addr.sin_port        = htons(params->data_port);
	data->addr.sin_addr.s_addr = inet_addr(params->dst_ip);
	data->socket               = socket(AF_INET, SOCK_DGRAM, 0);
	if (data->socket < 0) {
		/* TODO: manejar error */
	}
}

static void init_log_dest(struct net_host *log, struct net_params *params)
{
	log->addr.sin_family      = AF_INET;
	log->addr.sin_port        = htons(params->data_port);
	log->addr.sin_addr.s_addr = inet_addr(params->dst_ip);
	log->socket               = socket(AF_INET, SOCK_DGRAM, 0);
	if (log->socket < 0) {
		/* TODO: manejar error */
	}
}

static void init_listen(struct net_host *listen, struct net_params *params)
{
	listen->addr.sin_family      = AF_INET;
	listen->addr.sin_port        = htons(params->data_port);
	listen->addr.sin_addr.s_addr = inet_addr(params->dst_ip);
	listen->socket               = socket(AF_INET, SOCK_DGRAM, 0);
	if (listen->socket < 0) {
		/* TODO: manejar error */
	}
}
