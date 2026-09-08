#include <stdint.h> /* uint */
#include <stdbool.h> /* bool */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h> /* xTaskGetTickCount(), vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <driver/i2c.h> /* i2c */
#include <portmacro.h> /* TickType_t */
#include <projdefs.h> /* pdMS_TO_TICKS() */

#include "am2315c.h"

#define TAG "AM2315C"

#define NULL_ADDR        0xFF
#define RD_MIN_PERIOD_MS 2000
#define RD_WAIT_MS       80
#define RD_TOUT_MS       15
#define ST_BUSY_BIT      (1 << 7) /* 0x80 */


enum dev_state {
	DEV_READY_ST,
	DEV_WAIT_ST,
};

struct am2315c_dev {
	uint8_t        addr;
	enum dev_state st;
	uint16_t       waited_ms;
	uint16_t       period_ms;
	TickType_t     start_ticks;
	TickType_t     limit_ticks;
	float          hum;
	float          temp;
	esp_err_t      last_err;
};

struct am2315c_cfg {
	struct am2315c_dev devs[AM2315C_MAX_NUM_DEV];
	uint8_t ndevs;
};

static struct am2315c_cfg am2315c;

static esp_err_t read_status(struct am2315c_dev *dev, uint8_t *v);
static esp_err_t trigger_measurement(struct am2315c_dev *dev);
static esp_err_t read_measurement(struct am2315c_dev *dev, uint8_t *v);
static esp_err_t parse_measurement(struct am2315c_dev *dev, uint8_t *v);
static inline void timer_restart(struct am2315c_dev *dev, uint32_t limit_ms);
static inline bool timer_elapsed(struct am2315c_dev *dev);
static inline uint8_t crc8(const uint8_t *v, uint8_t len);


void am2315c_init(void)
{
	uint8_t i;
	struct am2315c_dev *dev;

	am2315c.ndevs = 0;
	for (i = 0; i < AM2315C_MAX_NUM_DEV; i++) {
		dev = &am2315c.devs[i];
		dev->addr        = NULL_ADDR;
		dev->st          = DEV_READY_ST;
		dev->waited_ms   = 0;
		dev->period_ms   = RD_MIN_PERIOD_MS;
		dev->start_ticks = xTaskGetTickCount();
		dev->limit_ticks = pdMS_TO_TICKS(RD_MIN_PERIOD_MS);
		dev->hum         = 0;
		dev->temp        = 0;
	}
}

void am2315c_add_dev(uint8_t addr, uint16_t period_ms)
{
	struct am2315c_dev *dev;
	uint8_t i;
	bool addr_exists = false;

	if (period_ms < RD_MIN_PERIOD_MS) {
		ESP_LOGE(TAG, "El periodo no puede ser inferior a %d. Ajustado.",
				(int) RD_MIN_PERIOD_MS);
		period_ms = RD_MIN_PERIOD_MS;
	}

	if (am2315c.ndevs >= AM2315C_MAX_NUM_DEV) {
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
			dev->period_ms = period_ms;
			dev->start_ticks = xTaskGetTickCount();
			dev->limit_ticks = pdMS_TO_TICKS(period_ms);
			am2315c.ndevs++;
			ESP_LOGI(TAG, "Añadido dispositivo. Dir: 0x%X", addr);
		}
	}
}

esp_err_t am2315c_get_hum(uint8_t addr, float *hum)
{
	struct am2315c_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < am2315c.ndevs) && (found == false)) {
		dev = &am2315c.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK)
				*hum = dev->hum;
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGW(TAG, "[get_hum] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGW(TAG, "[get_hum] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t am2315c_get_temp(uint8_t addr, float *temp)
{
	struct am2315c_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < am2315c.ndevs) && (found == false)) {
		dev = &am2315c.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK)
				*temp = dev->temp;
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGW(TAG, "[get_temp] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGW(TAG, "[get_temp] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t am2315c_read_all_devs(void)
{
	struct am2315c_dev *dev;
	uint8_t   i;
	uint8_t   st_reg;
	uint8_t   data[7];
	esp_err_t error = ESP_OK;
	esp_err_t last_error = ESP_OK;

	for (i = 0; i < am2315c.ndevs; i++) {
		dev = &am2315c.devs[i];
		if (dev->addr == NULL_ADDR)
			continue;

		if (timer_elapsed(dev) == false)
			continue;

		error = ESP_OK;

		switch (dev->st) {
		case DEV_READY_ST:
			/* 1. Mandar comando de leer registro.
			 * 2. Establecer temporizador a esperar 80 ms
			 * 3. Transitar a estado WAIT */
			error = trigger_measurement(dev);
			if (error == ESP_OK) {
				timer_restart(dev, RD_WAIT_MS);
				dev->st = DEV_WAIT_ST;
			} else {
				ESP_LOGE(TAG, "[addr: %x] Error al disparar medición: %s",
						dev->addr,
						esp_err_to_name(error));
			}
			break;
		case DEV_WAIT_ST:
			/*
			 * 1. Leer registro de estado
			 * 2. Comprobar si el bit 7 está a 0
			 * 3. Si está a 0, leer los datos,
			 * 4. Si no está a 0, esperar otros 80 ms
			 */
			dev->waited_ms += RD_WAIT_MS;
			error = read_status(dev, &st_reg);
			if ((error == ESP_OK) && ((st_reg & ST_BUSY_BIT) == 0)) {
				error = read_measurement(dev, data);
				if (error == ESP_OK) {
					error = parse_measurement(dev, data);
				} else {
					ESP_LOGE(TAG, "[addr: %x] Error al leer medición: %s",
							dev->addr,
							esp_err_to_name(error));
				}
				/* TODO: calcular desviación causada por esperas y restarla al
				 * nuevo tiempo de espera. Para esto está dev.waited_ms */
				if (error == ESP_OK) {
					timer_restart(dev, dev->period_ms);
					dev->st = DEV_READY_ST;
				} else {
					ESP_LOGE(TAG, "[addr: %x] Error al parsear muestra: %s",
							dev->addr,
							esp_err_to_name(error));
				}
			} else if (error == ESP_OK){
				timer_restart(dev, RD_WAIT_MS);
			} else {
				ESP_LOGE(TAG, "[addr: %x] Error al leer registro de estado: %s",
						dev->addr,
						esp_err_to_name(error));
			}
			break;
		default:
			break;
		}

		dev->last_err = error;
		if (error != ESP_OK)
			last_error = error;
	}

	return last_error;
}

TickType_t am2315c_get_min_wait(void)
{
	struct am2315c_dev *dev;
	TickType_t min_wait = pdMS_TO_TICKS(RD_MIN_PERIOD_MS);
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
			remaining = (dev->limit_ticks - elapsed);

		if (remaining < min_wait)
			min_wait = remaining;
	}

	return min_wait;
}

esp_err_t am2315c_get_dev_err(uint8_t addr)
{
	struct am2315c_dev *dev;
	esp_err_t error = ESP_FAIL;
	uint8_t   i     = 0;
	bool      found = 0;

	while ((i < am2315c.ndevs) && (found ==false)) {
		dev = &am2315c.devs[i];
		if (dev->addr == addr) {
			found = true;
			error = dev->last_err;
		}
		i++;
	}

	return error;
}


/* Funciones estáticas */

esp_err_t read_status(struct am2315c_dev *dev, uint8_t *v)
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

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(RD_TOUT_MS));

	i2c_cmd_link_delete(cmd);

	return error;
}

esp_err_t trigger_measurement(struct am2315c_dev *dev)
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

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(RD_TOUT_MS));

	i2c_cmd_link_delete(cmd);

	return error;
}

esp_err_t read_measurement(struct am2315c_dev *dev, uint8_t *v)
{
	esp_err_t error;
	i2c_cmd_handle_t cmd;

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
	i2c_master_read(cmd, v, 6, I2C_MASTER_ACK);
	i2c_master_read(cmd, v + 6, 1, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(RD_TOUT_MS));

	i2c_cmd_link_delete(cmd);

	return error;
}

esp_err_t parse_measurement(struct am2315c_dev *dev, uint8_t *v)
{
	esp_err_t error = ESP_OK;
	/* 1 / 2^20 * 100 (hoja de datos, sección 8.1) */
	static const float hum_factor = 9.5367431640625e-5f;
	/* 1 / 2^20 * 200 (hoja de datos, sección 8.2) */
	static const float temp_factor = 1.9073486328125e-4f;

	uint8_t crc;
	uint32_t raw_hum;
	uint32_t raw_temp;

	crc = crc8(v, 6);
	if (crc != v[6]) {
		error = ESP_ERR_INVALID_CRC;
	} else {
		if ((v[0] & ST_BUSY_BIT) != 0) {
			error = ESP_FAIL;
		}
		else {
			raw_hum = ((uint32_t) v[1] << 12)
					| ((uint32_t) v[2] << 4)
					| ((uint32_t) v[3] >> 4);
			dev->hum = raw_hum * hum_factor;

			raw_temp = (((uint32_t) v[3] & 0x0F) << 16)
					  | ((uint32_t) v[4] << 8)
					  | ((uint32_t) v[5]);
			dev->temp = raw_temp * temp_factor - 50;
		}
	}

	return error;
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
