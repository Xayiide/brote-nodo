#include <string.h> /* strlen */
#include <assert.h> /* static_assert */

#include <freertos/FreeRTOS.h> /* Debe aparecer primero */
#include <freertos/event_groups.h> /* EventGroupHandle_t, xEventGroupSetBits */

#include <esp_event.h> /* esp_event_loop_create_default, esp_event_* */
#include <esp_wifi.h> /* esp_wifi_connect, */
#include <esp_log.h> /* ESP_LOG */

#include <lwip/ip4_addr.h> /* ip4addr_ntoa */


#include "wifi.h"

#if !defined(WIFI_SSID)
#error "WIFI_SSID no está definido"
#endif

#if !defined(WIFI_PASS)
#error "WIFI_PASS no está definido"
#endif

_Static_assert(sizeof(WIFI_SSID) > 1, "WIFI_SSID está vacío");
_Static_assert(sizeof(WIFI_PASS) > 1, "WIFI_PASS está vacío");

#define TAG                       "WIFI"
#define EXAMPLE_ESP_MAXIMUM_RETRY 10
#define WIFI_CONNECTED_BIT        BIT0

static EventGroupHandle_t s_wifi_event_group;

enum wifi_state {
	WIFI_DISCONNECTED,
	WIFI_CONNECTING,
	WIFI_CONNECTED
};

static enum wifi_state s_state = WIFI_DISCONNECTED;
static uint32_t s_retry_num = 0;

static void event_handler(void* arg, esp_event_base_t ev_base,
                                int32_t ev_id, void* ev_data)
{
	if (ev_base == WIFI_EVENT && ev_id == WIFI_EVENT_STA_START) {
		s_state = WIFI_CONNECTING;
		s_retry_num++;
		// log(LOG_DEBUG, LOG_WIFI, "Conectando");
		ESP_LOGI(TAG, "Conectando");
		esp_wifi_connect();
	}
	else if (ev_base == WIFI_EVENT && ev_id == WIFI_EVENT_STA_DISCONNECTED) {
		s_state = WIFI_DISCONNECTED;
		xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		// log(LOG_DEBUG, LOG_WIFI, "Desconectado, reintentando");
		ESP_LOGW(TAG, "Desconectado, reintentando (%u)", s_retry_num);
		vTaskDelay(pdMS_TO_TICKS(2000));
		esp_wifi_connect();
		s_retry_num++;
	}
	else if (ev_base == IP_EVENT && ev_id == IP_EVENT_STA_GOT_IP) {
		s_state = WIFI_CONNECTED;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
		s_retry_num = 0;
		ip_event_got_ip_t* event = (ip_event_got_ip_t*) ev_data;
		// log(LOG_DEBUG, LOG_WIFI, "Got IP");
		ESP_LOGI(TAG, "IP: %s", ip4addr_ntoa(&event->ip_info.ip));
	}
}

void wifi_init_sta(void)
{
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	EventBits_t        bits;
	wifi_config_t      wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASS
		},
	};

	ESP_LOGI(TAG, "WIFI_SSID: %s | WIFI_PASS: %s", WIFI_SSID, WIFI_PASS);

	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,
	                                           ESP_EVENT_ANY_ID,
	                                           &event_handler,
	                                           NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,
	                                           IP_EVENT_STA_GOT_IP,
	                                           &event_handler,
	                                           NULL));

	if (strlen((char *) wifi_config.sta.password)) {
		wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start());

	bits = xEventGroupWaitBits(s_wifi_event_group,
	                           WIFI_CONNECTED_BIT,
	                           pdFALSE,
	                           pdFALSE,
	                           portMAX_DELAY);

	if (bits & WIFI_CONNECTED_BIT) {
		// log(LOG_DEBUG, LOG_WIFI, "Conectado a SSID");
		ESP_LOGI(TAG, "Conectado al SSID %s", WIFI_SSID);
	}
	else {
		ESP_LOGE(TAG, "ERROR INESPERADO");
	}
}

