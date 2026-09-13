#include <stdint.h> /* uint */
#include <stdbool.h> /* bool */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskGetTickCount */
#include <driver/gpio.h> /* gpio_config */
#include <esp_log.h> /* esp_log */

#include "led.h"

#define TAG "LED"

#define LED1 16
#define OUTPUT_MASK ((1ULL << LED1))
#define PERIOD_MS 500

enum led_st {
	LED_ON = 1,
	LED_OFF = 0,
};

struct led_cfg {
	TickType_t  start_ticks;
	TickType_t  limit_ticks;
	enum led_st st;
};

static struct led_cfg led;

static void switch_led_st(void);

void led_init(void)
{
	gpio_config_t io_conf;

	io_conf.intr_type    = GPIO_INTR_DISABLE; /* Disable interrupts */
	io_conf.mode         = GPIO_MODE_OUTPUT; /* Output mode */
	io_conf.pin_bit_mask = OUTPUT_MASK; /* Bitmask de los pines */
	io_conf.pull_down_en = 0; /* Disable pull-down mode */
	io_conf.pull_up_en   = 1; /* Enable pull-up mode */
	gpio_config(&io_conf);

	led.start_ticks = xTaskGetTickCount();
	led.limit_ticks = pdMS_TO_TICKS(PERIOD_MS);
	led.st = LED_OFF;

	gpio_set_level(LED1, led.st);
}

void led_main(void)
{
	TickType_t now, elapsed;

	now = xTaskGetTickCount();
	elapsed = now - led.start_ticks;

	if (elapsed >= led.limit_ticks) {
		led.start_ticks = now;
		switch_led_st();
	}
}


/* Funciones estáticas */

void switch_led_st(void)
{
	if (led.st == LED_ON)
		led.st = LED_OFF;
	else
		led.st = LED_ON;

	gpio_set_level(LED1, led.st);
}
