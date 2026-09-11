#include <stdint.h> /* uint */
#include <string.h>

#include <sys/socket.h> /* sockaddr_in, AF_INET, socket, sendto */
#include <netinet/in.h> /* INET_ADDRSTRLEN */
#include <lwip/def.h> /* htons */
#include <lwip/inet.h> /* inet_addr */
#include <tcpip_adapter.h> /* tcpip_adapter_init, dhcpc_stop */

#include <esp_log.h> /* ESP_LOG */

#include "net.h"

#define TAG "NET"

static struct sockaddr_in data_dest, log_dest;
static int data_sock, log_sock;
char ip4[INET_ADDRSTRLEN];

static void set_static_ip(void);

void net_init(const char *ip, uint16_t data_port, uint16_t log_port)
{
	if ((ip == NULL) || (data_port == 0) || (log_port == 0)) {
		ESP_LOGE(TAG, "IP or ports invalid");
		return;
	}

	tcpip_adapter_init();

	set_static_ip();

	data_dest.sin_family      = AF_INET;
	data_dest.sin_port        = htons(data_port);
	data_dest.sin_addr.s_addr = inet_addr(ip);

	log_dest.sin_family      = AF_INET;
	log_dest.sin_port        = htons(log_port);
	log_dest.sin_addr.s_addr = inet_addr(ip);

	data_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (data_sock < 0) {
		/* TODO: manejar error */
	}

	log_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (log_sock < 0) {
		/* TODO: manejar error */
	}
}

void net_udp_send(const uint8_t *data, size_t len)
{
	sendto(data_sock, data, len,
	       0, (struct sockaddr *) &data_dest, sizeof(data_dest));
}

void net_log_send(const uint8_t *data, size_t len)
{
	sendto(log_sock, data, len,
	       0, (struct sockaddr *) &log_dest, sizeof(log_dest));
}

void set_static_ip(void)
{
	tcpip_adapter_ip_info_t ip_info;

	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);

	/* TODO: poner la IP en config */
	IP4_ADDR(&ip_info.ip, 172, 20, 10, 200);
	IP4_ADDR(&ip_info.gw, 172, 20, 10, 1);
	IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);
}

