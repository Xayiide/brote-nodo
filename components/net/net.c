#include <stdint.h> /* uint */
#include <string.h>

#include <sys/socket.h> /* sockaddr_in, AF_INET, socket, sendto */
#include <netinet/in.h> /* INET_ADDRSTRLEN */
#include <lwip/def.h> /* htons */
#include <lwip/inet.h> /* inet_addr */

#include <esp_log.h> /* ESP_LOG */

#include "net.h"

#define TAG "NET"

static struct sockaddr_in data_dest, log_dest;
static int data_sock, log_sock;
char ip4[INET_ADDRSTRLEN];

void net_init(const char *ip, uint16_t data_port, uint16_t log_port)
{
	if ((ip == NULL) || (data_port == 0) || (log_port == 0)) {
		ESP_LOGE(TAG, "IP or ports invalid");
		return;
	}

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
