#include <stdint.h> /* uint */
#include <stdbool.h> /* bool */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskGetTickCount(), vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <driver/i2c.h> /* i2c */
#include <portmacro.h> /* TickType_t, portMAX_DELAY */
#include <projdefs.h> /* pdMS_TO_TICKS() */

#include "am2315c.h"

#define TAG "AM2315C"

#define MAX_NUM_DEV  1
#define NULL_ADDR    0xFF
#define RD_PERIOD_MS 2000
#define RD_WAIT_MS   80
#define ST_BUSY_BIT  (1 << 7) /* 0x80 */


enum dev_state {
	DEV_READY_ST,
	DEV_WAIT_ST,
};

struct am2315c_dev {
	uint8_t        addr;
	enum dev_state st;
	uint16_t       waited_ms;
	TickType_t     start_ticks;
	TickType_t     limit_ticks;
	float          hum;
	float          temp;
};

struct am2315c_cfg {
	struct am2315c_dev devs[MAX_NUM_DEV];
	uint8_t ndevs;
};

static struct am2315c_cfg am2315c;

static void main_task(void *p);
static void read_all_devs(void);
static TickType_t get_min_wait(void);
static void read_status(struct am2315c_dev *dev, uint8_t *v);
static void trigger_measurement(struct am2315c_dev *dev);
static void read_measurement(struct am2315c_dev *dev, uint8_t *v);
static void parse_measurement(struct am2315c_dev *dev, uint8_t *v);
static inline void timer_restart(struct am2315c_dev *dev, uint32_t limit_ms);
static inline bool timer_elapsed(struct am2315c_dev *dev);
static inline uint8_t crc8(const uint8_t *v, uint8_t len);


void am2315c_init(void)
{
	uint8_t i;
	struct am2315c_dev *dev;

	am2315c.ndevs = 0;
	for (i = 0; i < MAX_NUM_DEV; i++) {
		dev = &am2315c.devs[i];
		dev->addr        = NULL_ADDR;
		dev->st          = DEV_READY_ST;
		dev->waited_ms   = 0;
		dev->start_ticks = (TickType_t) 0;
		dev->limit_ticks = (TickType_t) 0;
		dev->hum         = 0;
		dev->temp        = 0;
	}

	xTaskCreate(&main_task, "am2315c_task", 4096, NULL, 1, NULL);
}

void am2315c_add_dev(uint8_t addr, uint16_t period_ms)
{
	struct am2315c_dev *dev;
	uint8_t i;
	bool addr_exists = false;

	if (period_ms < RD_PERIOD_MS) {
		ESP_LOGE(TAG, "El periodo no puede ser inferior a %d. Ajustado.", (int) RD_PERIOD_MS);
		period_ms = RD_PERIOD_MS;
	}

	if (am2315c.ndevs >= MAX_NUM_DEV) {
		ESP_LOGE(TAG, "No se pueden añadir más sensores, máximo alcanzado");
	}
	else {
		for (i = 0; i < am2315c.ndevs; i++)
			if (am2315c.devs[i].addr == addr)
				addr_exists = true;

		if (addr_exists == true) {
			ESP_LOGE(TAG, "Ya existe un dispositivo con dir. %d", addr);
		}
		else {
			dev = &am2315c.devs[am2315c.ndevs];
			dev->addr = addr;
			dev->start_ticks = xTaskGetTickCount();
			dev->limit_ticks = pdMS_TO_TICKS(period_ms);
			am2315c.ndevs++;
			ESP_LOGI(TAG, "Añadido dispositivo. Dir: %d", addr);
		}
	}
}

float am2315c_hum(uint8_t addr)
{
	float res = -1;
	uint8_t i;

	for (i = 0; i < am2315c.ndevs; i++) {
		if (am2315c.devs[i].addr == addr)
			res = am2315c.devs[i].hum;
	}

	if (res == -1)
		ESP_LOGW(TAG, "No existe un dev con la dir. %d", addr);

	return res;
}

float am2315c_temp(uint8_t addr)
{
	float res = -1;
	uint8_t i;

	for (i = 0; i < am2315c.ndevs; i++) {
		if (am2315c.devs[i].addr == addr)
			res = am2315c.devs[i].temp;
	}

	if (res == -1)
		ESP_LOGW(TAG, "No existe un dev con la dir. %d", addr);

	return res;
}


/* Funciones estáticas */

void main_task(void *p)
{
	TickType_t min_wait;

	for (;;) {
		min_wait = get_min_wait();

		if (min_wait > 0) {
			vTaskDelay(min_wait);
		}

		read_all_devs();
	}
}

TickType_t get_min_wait(void)
{
	struct am2315c_dev *dev;
	TickType_t min_wait = pdMS_TO_TICKS(RD_PERIOD_MS);
	TickType_t now, elapsed, remaining;
	uint8_t    i;

	now = xTaskGetTickCount();

	for (i = 0; i < am2315c.ndevs; i++) {
		dev = &am2315c.devs[i];

		if (dev->addr == NULL_ADDR)
			continue;

		elapsed = now - dev->start_ticks;
		if (elapsed >= dev->limit_ticks)
			remaining = 0;
		else
			remaining = dev->limit_ticks;

		if (remaining < min_wait)
			min_wait = remaining;
	}

	return min_wait;
}

void read_all_devs(void)
{
	struct am2315c_dev *dev;
	uint8_t i;
	uint8_t st_reg;
	uint8_t data[7];

	for (i = 0; i < am2315c.ndevs; i++) {
		dev = &am2315c.devs[i];
		if (dev->addr == NULL_ADDR)
			continue;

		if (timer_elapsed(dev) == false)
			continue;

		switch (dev->st) {
		case DEV_READY_ST:
			/* 1. Mandar comando de leer registro.
			 * 2. Establecer temporizador a esperar 80 ms
			 * 3. Transitar a estado WAIT */
			trigger_measurement(dev);
			timer_restart(dev, RD_WAIT_MS);
			dev->st = DEV_WAIT_ST;
			break;
		case DEV_WAIT_ST:
			/*
			 * 1. Leer registro de estado
			 * 2. Comprobar si el bit 7 está a 0
			 * 3. Si está a 0, leer los datos,
			 * 4. Si no está a 0, esperar otros 80 ms
			 */
			dev->waited_ms += RD_WAIT_MS;
			read_status(dev, &st_reg);
			if ((st_reg & ST_BUSY_BIT) == 0) {
				read_measurement(dev, data);
				parse_measurement(dev, data);
				/* TODO: calcular desviación causada por esperas y restarla al
				 * nuevo tiempo de espera. Para esto está dev.waited_ms */
				timer_restart(dev, RD_PERIOD_MS);
				dev->st = DEV_READY_ST;
			} else {
				timer_restart(dev, RD_WAIT_MS);
			}
			break;
		default:
			break;
		}

	}
}

void read_status(struct am2315c_dev *dev, uint8_t *v)
{
	esp_err_t        error;
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, 0x71, true);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
	i2c_master_read(cmd, v, 1, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, 2000 / portTICK_PERIOD_MS);
	if (error != ESP_OK) {
		ESP_LOGE(TAG, "Error al leer registro de estado: %s",
		    esp_err_to_name(error));
	}

	i2c_cmd_link_delete(cmd);
}

void trigger_measurement(struct am2315c_dev *dev)
{
	esp_err_t error;
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, 0xAC, true);
	i2c_master_write_byte(cmd, 0x33, true);
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, 2000 / portTICK_PERIOD_MS);
	if (error != ESP_OK) {
		ESP_LOGE(TAG, "Error al disparar medición: %s",
		    esp_err_to_name(error));
	}

	i2c_cmd_link_delete(cmd);
}

void read_measurement(struct am2315c_dev *dev, uint8_t *v)
{
	esp_err_t error;
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
	i2c_master_read(cmd, v, 6, I2C_MASTER_ACK);
	i2c_master_read(cmd, v + 6, 1, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, 2000 / portTICK_PERIOD_MS);
	if (error != ESP_OK) {
		ESP_LOGE(TAG, "Error al leer medición: %s", esp_err_to_name(error));
	}

	i2c_cmd_link_delete(cmd);
}

void parse_measurement(struct am2315c_dev *dev, uint8_t *v)
{

	/* 1 / 2^20 * 100 (hoja de datos, sección 8.1) */
	static const float hum_factor = 9.5367431640625e-5f;
	/* 1 / 2^20 * 200 (hoja de datos, sección 8.2) */
	static const float temp_factor = 1.9073486328125e-4f;

	uint8_t crc;
	uint32_t raw_hum;
	uint32_t raw_temp;

	if ((v[0] & ST_BUSY_BIT) != 0) {
		return;
	}

	crc = crc8(v, 6);
	if (crc != v[6]) {
		return;
	}

	raw_hum = ((uint32_t) v[1] << 12)
			| ((uint32_t) v[2] << 4)
			| ((uint32_t) v[3] >> 4);
	dev->hum = raw_hum * hum_factor;

	raw_temp = (((uint32_t) v[3] & 0x0F) << 16)
			  | ((uint32_t) v[4] << 8)
			  | ((uint32_t) v[5]);
	dev->temp = raw_temp * temp_factor - 50;
}

inline void timer_restart(struct am2315c_dev *dev, uint32_t limit_ms)
{
	dev->start_ticks = xTaskGetTickCount();
	dev->limit_ticks = pdMS_TO_TICKS(limit_ms);
}

inline bool timer_elapsed(struct am2315c_dev *dev)
{
	bool res;

	res = (xTaskGetTickCount() - dev->start_ticks) >= dev->limit_ticks;

	return res;
}

inline uint8_t crc8(const uint8_t *p, uint8_t len)
{
	uint8_t crc = 0xFF;
	uint8_t i;

	while (len--) {
		crc ^= *p++;
		for (i = 0; i < 8; i++) {
			if (crc & 0x80) {
				crc <<= 1;
				crc ^= 0x31; /* Polinomio: hoja de datos 7.4 punto 4 */
			} else {
				crc <<= 1;
			}
		}
	}

	return crc;
}
