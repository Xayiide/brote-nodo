#include <stdint.h> /* uint */
#include <stdbool.h> /* bool */
#include <stddef.h> /* NULL */

#include <FreeRTOS.h> /* Es necesario ponerlo primero */
#include <task.h>    /* xTaskGetTickCount(), vTaskDelay */
#include <esp_log.h> /* ESP_LOG */
#include <esp_err.h> /* esp_err */
#include <driver/i2c.h> /* i2c */
#include <portmacro.h> /* TickType_t */
#include <projdefs.h> /* pdMS_TO_TICKS() */

#include "veml7700.h"

#define TAG "VEML7700"

#define ALS_COUNT_LO     100
#define ALS_COUNT_HI     45000
#define RD_MIN_PERIOD_MS 1000
#define RD_TOUT_MS       15
#define NULL_ADDR        0xFF

enum dev_state {
	DEV_READY_ST,
	DEV_WAIT_IT_ST,
};

enum cmd_code {
	CMD_ALS_CONFIG   = 0x0,
	CMD_ALS_HIGH_THR = 0x1,
	CMD_ALS_LOW_THR  = 0x2,
	CMD_PWR_SAVE     = 0x3,
	CMD_ALS_DATA     = 0x4,
	CMD_WHITE_DATA   = 0x5,
	CMD_INT_STATUS   = 0x6,
	CMD_ID_REGISTER  = 0x7,
};

#define ALS_GAIN_NUM_OPT 4
enum als_gain {
	ALS_GAIN_2   = 0x1,
	ALS_GAIN_1   = 0x0,
	ALS_GAIN_1_4 = 0x3,
	ALS_GAIN_1_8 = 0x2,
};
static enum als_gain gain_values[ALS_GAIN_NUM_OPT] = {
	ALS_GAIN_2,
	ALS_GAIN_1,
	ALS_GAIN_1_4,
	ALS_GAIN_1_8,
};

#define ALS_IT_NUM_OPT 6
enum als_it {
	ALS_IT_25MS  = 0xC,
	ALS_IT_50MS  = 0x8,
	ALS_IT_100MS = 0x0,
	ALS_IT_200MS = 0x1,
	ALS_IT_400MS = 0x2,
	ALS_IT_800MS = 0x3,
};
static enum als_it it_values[ALS_IT_NUM_OPT] = {
	ALS_IT_800MS,
	ALS_IT_400MS,
	ALS_IT_200MS,
	ALS_IT_100MS,
	ALS_IT_50MS,
	ALS_IT_25MS,
};

#define ALS_PERS_NUM_OPT 4
enum als_pers {
	ALS_PERS_1 = 0x0,
	ALS_PERS_2 = 0x1,
	ALS_PERS_4 = 0x2,
	ALS_PERS_8 = 0x3,
};

#define PWR_SAV_NUM_OPT 4
enum pwr_mode {
	PWR_SAV_MODE_1 = 0x0,
	PWR_SAV_MODE_2 = 0x1,
	PWR_SAV_MODE_3 = 0x2,
	PWR_SAV_MODE_4 = 0x3,
};

enum int_st {
	INT_DISABLED = 0x0,
	INT_ENABLED  = 0x1,
};

struct veml7700_params {
	enum als_gain gain;
	enum als_it   it;
	enum als_pers pers;
	enum int_st   int_status;
	enum pwr_mode pwr_mode;
	float         res;
	uint32_t      max_lux;
};

struct veml7700_dev {
	struct veml7700_params params;
	uint8_t                addr;
	enum dev_state         st;
	uint16_t               period_ms;
	TickType_t             start_ticks;
	TickType_t             limit_ticks;
	float                  lx;
	float                  wh;
	uint16_t               raw_lx;
	uint16_t               raw_wh;
	esp_err_t              last_err;
	bool                   sample_ready;
};

struct veml7700_cfg {
	struct veml7700_dev devs[VEML7700_MAX_NUM_DEV];
	uint8_t ndevs;
};

static struct veml7700_cfg veml7700;


static esp_err_t  poll_dev(struct veml7700_dev *dev);
static esp_err_t  poll_dev_ready(struct veml7700_dev *dev);
static esp_err_t  poll_dev_wait_it(struct veml7700_dev *dev);
static bool       autorange(struct veml7700_dev *dev, uint16_t als_count);
static int8_t     gain_to_idx(enum als_gain gain);
static int8_t     it_to_idx(enum als_it it);
static uint16_t   it_to_ms(enum als_it it);
static void       get_default_config(struct veml7700_dev *dev);
static int32_t    get_max_illumination(enum als_gain gain, enum als_it it_ms);
static float      get_resolution(enum als_gain gain, enum als_it it_ms);
static esp_err_t  set_and_send_config(struct veml7700_dev *dev);
static esp_err_t  read_reg(struct veml7700_dev *dev,
                           enum cmd_code        reg,
                           uint16_t            *v);
static esp_err_t  write_reg(struct veml7700_dev *dev,
                            enum cmd_code        reg,
                            uint16_t             v);
static inline void timer_restart(struct veml7700_dev *dev, uint32_t limit_ms);
static inline bool timer_elapsed(struct veml7700_dev *dev);


/* Funciones públicas */

void veml7700_init(void)
{
	uint8_t i;
	struct veml7700_dev *dev;

	veml7700.ndevs = 0;
	for (i = 0; i < VEML7700_MAX_NUM_DEV; i++) {
		dev = &veml7700.devs[i];
		dev->addr         = NULL_ADDR;
		dev->st           = DEV_READY_ST;
		dev->period_ms    = RD_MIN_PERIOD_MS;
		dev->start_ticks  = xTaskGetTickCount();
		dev->limit_ticks  = pdMS_TO_TICKS(RD_MIN_PERIOD_MS);
		dev->lx           = 0;
		dev->wh           = 0;
		dev->raw_lx       = 0;
		dev->raw_wh       = 0;
		dev->last_err     = ESP_FAIL;
		dev->sample_ready = false;
		get_default_config(dev);
	}
}

void veml7700_add_dev(uint8_t addr, uint16_t period_ms)
{
	struct veml7700_dev *dev;
	uint8_t i;
	bool addr_exists = false;

	if (period_ms < RD_MIN_PERIOD_MS) {
		ESP_LOGW(TAG, "El periodo no puede ser inferior a %d. Ajustado.",
				(int) RD_MIN_PERIOD_MS);
		period_ms = RD_MIN_PERIOD_MS;
	}

	if (veml7700.ndevs >= VEML7700_MAX_NUM_DEV) {
		ESP_LOGE(TAG, "No se pueden añadir más sensores, máximo alcanzado");
	}
	else {
		for (i = 0; i < veml7700.ndevs; i++)
			if (veml7700.devs[i].addr == addr)
				addr_exists = true;

		if (addr_exists == true) {
			ESP_LOGE(TAG, "Ya existe un dispositivo con dir. 0x%02X", addr);
		}
		else {
			dev = &veml7700.devs[veml7700.ndevs];
			dev->addr = addr;
			dev->period_ms = period_ms;
			dev->start_ticks = xTaskGetTickCount();
			dev->limit_ticks = pdMS_TO_TICKS(period_ms);
			set_and_send_config(&veml7700.devs[veml7700.ndevs]);
			veml7700.ndevs++;
			ESP_LOGI(TAG, "Añadido dispositivo. Dir: 0x%02X", addr);
		}
	}

	/* NOTA: En la hoja de datos dice que al activar el sensor, hay
	 * que esperar al menos 2.5 ms antes de tomar la primera medida.
	 * Mientras se tenga RD_PERIOD_MS a 1000, se espera un segundo,
	 * pero si se baja a 1 ó 2, entonces hay que implementar lógica
	 * para esperar los 2.5 ms antes de medir. */
}



esp_err_t veml7700_get_lux(uint8_t addr, float *lx)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK)
				*lx = dev->lx;
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGE(TAG, "[get_lux] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGE(TAG, "[get_lux] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t veml7700_get_white(uint8_t addr, float *wh)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK)
				*wh = dev->wh;
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGE(TAG, "[get_white] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGE(TAG, "[get_white] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t veml7700_get_cfg(uint8_t addr, uint16_t *it, uint8_t *gain)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK) {
				*it = it_to_ms(dev->params.it);
				*gain = dev->params.gain;
			}
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGE(TAG, "[get_cfg] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGE(TAG, "[get_cfg] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t veml7700_get_res(uint8_t addr, float *r)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t  i      = 0;
	bool     found  = 0;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK) {
				*r = dev->params.res;
			}
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGE(TAG, "[get_res] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGE(TAG, "[get_res] Error: %s", esp_err_to_name(error));

	return error;
}

esp_err_t veml7700_get_raw(uint8_t addr, uint16_t *raw_lx, uint16_t *raw_wh)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_OK;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			if (dev->last_err == ESP_OK) {
				*raw_lx = dev->raw_lx;
				*raw_wh = dev->raw_wh;
			}
			error = dev->last_err;
		}
		i++;
	}

	if (found == false)
		ESP_LOGE(TAG, "[get_raw] No existe un dev con la dir. 0x%02X", addr);

	if (error != ESP_OK)
		ESP_LOGE(TAG, "[get_raw] Error: %s", esp_err_to_name(error));

	return error;
}

/*
 * @brief Transita la máquina de estados de cada dispositivo
 * @param None
 * @return esp_err_t
 */
esp_err_t veml7700_poll(void)
{
	struct veml7700_dev *dev;
	uint8_t              i;
	esp_err_t            error = ESP_OK;
	esp_err_t            last_error = ESP_OK;

	for (i = 0; i < veml7700.ndevs; i++) {
		dev = &veml7700.devs[i];
		if (dev->addr == NULL_ADDR)
			continue;

		if (timer_elapsed(dev) == false)
			continue;

		error = poll_dev(dev);

		dev->last_err = error;
		if (error != ESP_OK)
			last_error = error;

		if (error != ESP_OK)
			timer_restart(dev, dev->period_ms);
	}

	return last_error;
}

TickType_t veml7700_get_min_wait(void)
{
	struct veml7700_dev *dev;
	TickType_t min_wait = pdMS_TO_TICKS(RD_MIN_PERIOD_MS);
	TickType_t now, elapsed, remaining;
	uint8_t    i;

	now = xTaskGetTickCount();

	for (i = 0; i < veml7700.ndevs; i++) {
		dev = &veml7700.devs[i];

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

esp_err_t veml7700_get_dev_err(uint8_t addr)
{
	struct veml7700_dev *dev;
	esp_err_t error = ESP_FAIL;
	uint8_t   i     = 0;
	bool      found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			error = dev->last_err;
		}
		i++;
	}

	return error;
}

bool veml7700_is_sample_ready(uint8_t addr)
{
	struct veml7700_dev *dev;
	bool    ready = false;
	uint8_t i     = 0;
	bool    found = false;

	while ((i < veml7700.ndevs) && (found == false)) {
		dev = &veml7700.devs[i];
		if (dev->addr == addr) {
			found = true;
			ready = dev->sample_ready;
		}
		i++;
	}

	if (found == false) {
		ESP_LOGE(TAG, "[is_sample_ready] No existe un dev con la dir. 0x%02X",
				addr);
	}

	return ready;
}


/* Funciones estáticas */

esp_err_t poll_dev(struct veml7700_dev *dev)
{
	esp_err_t error = ESP_OK;

	dev->sample_ready = false;

	/* Cuando se cambia la configuración, hay que esperar su IT para que
	 * la lectura sea válida */
	switch (dev->st) {
	case DEV_READY_ST:
		error = poll_dev_ready(dev);
		break;
	case DEV_WAIT_IT_ST:
		error = poll_dev_wait_it(dev);
		break;
	default:
		break;
	}

	return error;
}

esp_err_t poll_dev_ready(struct veml7700_dev *dev)
{
	esp_err_t error = ESP_OK;
	uint8_t   cfg_changed;
	uint16_t  als_count, white_count;

	error = read_reg(dev, CMD_ALS_DATA, &als_count);
	if (error == ESP_OK) {
		cfg_changed = autorange(dev, als_count);
		if (cfg_changed) {
			/* Si ha cambiado la config:
			 * 1. Configurar el sensor con la nueva config
			 * 2. Establecer temporizador a esperar IT_TIME y reiniciarlo
			 * 3. Transitar a estado WAIT_IT
			 */
			error = set_and_send_config(dev);
			if (error == ESP_OK) {
				timer_restart(dev, it_to_ms(dev->params.it));
				dev->st = DEV_WAIT_IT_ST;
			} else {
				ESP_LOGE(TAG, "[addr: %x] Error enviando la config: %s",
						dev->addr,
						esp_err_to_name(error));
			}
		} else {
			/* No ha cambiado la config: als_count es válido */
			error = read_reg(dev, CMD_WHITE_DATA, &white_count);
			if (error == ESP_OK) {
				dev->raw_lx = als_count;
				dev->raw_wh = white_count;
				dev->lx = als_count   * dev->params.res;
				dev->wh = white_count * dev->params.res;
				dev->sample_ready = true;
				timer_restart(dev, dev->period_ms);
				/* No se ha modificado la config: no hace falta cambiar de
				 * estado ni esperar IT_TIME */
			} else {
				ESP_LOGE(TAG, "[addr: %x] [R:ALS] Error al leer registro: %s",
						dev->addr,
						esp_err_to_name(error));
			}
		}
	} else {
		ESP_LOGE(TAG, "[addr: %x] [R:WHT] Error al leer registro: %s",
				dev->addr,
				esp_err_to_name(error));
	}

	return error;
}

esp_err_t poll_dev_wait_it(struct veml7700_dev *dev)
{
	esp_err_t error = ESP_OK;
	uint16_t  als_count, white_count;
	uint32_t  it_ms, remaining_ms;

	/* Esperar a ver si ha pasado ya IT_TIME y se puede consumir
	 * la lectura */
	error = read_reg(dev, CMD_ALS_DATA, &als_count);
	if (error == ESP_OK) {
		error = read_reg(dev, CMD_WHITE_DATA, &white_count);
	}

	if (error == ESP_OK) {
		dev->raw_lx = als_count;
		dev->raw_wh = white_count;
		dev->lx = als_count * dev->params.res;
		dev->wh = white_count * dev->params.res;
		dev->sample_ready = true;

		/* Como ya ha pasado el IT_TIME, se vuelve a esperar el tiempo
		 * normal configurado, corrigiendo la desviación acumulada:
		 * Si se ha esperado 100MS de IT, y ahora se espera 1000 de tiempo
		 * configurado, la demora será de 1100 ms en leer, en lugar de
		 * 1000. Esa desviación se acumula con cada nueva configuración.
		 * Pero no se puede hacer simplemente
		 * pit_ms_to_ticks(RD_PERIOD_MS - it_to_ms(dev->params.it)) porque
		 * si RD_PERIOD_MS es pequeño, la operación desbordaría. Así que
		 * se calculan los milisegundos restantes */
		it_ms = it_to_ms(dev->params.it);
		remaining_ms = (it_ms < dev->period_ms) ?
				(dev->period_ms- it_ms) : 0;
		timer_restart(dev, remaining_ms);
		dev->st = DEV_READY_ST;
	} else {
		ESP_LOGE(TAG, "[addr: %x] [WAIT_IT] Error al leer registro: %s",
				dev->addr,
				esp_err_to_name(error));
	}

	return error;
}

bool autorange(struct veml7700_dev *dev, uint16_t als_count)
{
	enum als_gain old_gain = dev->params.gain;
	enum als_it old_it = dev->params.it;
	int8_t gain_idx = gain_to_idx(dev->params.gain);
	int8_t it_idx   = it_to_idx(dev->params.it);
	//uint8 changed  = 0;

	if ((gain_idx < 0) || (it_idx < 0))
		return false;

	/* Comprobar si la lectura es baja para aumentar la sensibilidad */
	if (als_count <= ALS_COUNT_LO) {
		/* Incrementar ganancia si no está al máximo */
		if (dev->params.gain != ALS_GAIN_2) {
			dev->params.gain = gain_values[gain_idx - 1];
			//changed = 1;
		}
		else {
			/* Ganancia está al máximo. Incrementar IT si no está al máximo */
			if (dev->params.it != ALS_IT_800MS) {
				dev->params.it = it_values[it_idx - 1];
				//changed = 1;
			}
		}
	}
	/* Comprobar si la lectura está demasiado alta para bajar sensibilidad */
	else if (als_count >= ALS_COUNT_HI) {
		/* Reducimos IT si no está al mínimo */
		if (dev->params.it != ALS_IT_25MS) {
			dev->params.it = it_values[it_idx + 1];
			//changed = 1;
		}
		else {
			/* It está al mínimo. Reducir ganancia si no está al mínimo */
			if (dev->params.gain != ALS_GAIN_1_8) {
				dev->params.gain = gain_values[gain_idx + 1];
				//changed = 1;
			}
		}
	}

	return (bool) (dev->params.gain != old_gain) || (dev->params.it != old_it);
}

int8_t gain_to_idx(enum als_gain gain)
{
	int8_t  idx   = -1;
	uint8_t i     = 0;
	bool    found = false;

	while ((i < ALS_GAIN_NUM_OPT) && (found == false)) {
		if (gain_values[i] == gain) {
			found = true;
			idx = i;
		}
		i++;
	}


	return idx;
}

int8_t it_to_idx(enum als_it it)
{
	int8_t  idx   = -1;
	uint8_t i     = 0;
	bool    found = false;

	while ((i < ALS_IT_NUM_OPT) && (found == false)) {
		if (it_values[i] == it) {
			found = true;
			idx = i;
		}
		i++;
	}

	return idx;
}

uint16_t it_to_ms(enum als_it it)
{
	switch(it) {
	case ALS_IT_25MS:  return 25;
	case ALS_IT_50MS:  return 50;
	case ALS_IT_100MS: return 100;
	case ALS_IT_200MS: return 200;
	case ALS_IT_400MS: return 400;
	case ALS_IT_800MS: return 800;
	default:           return 800; /* Si it no cuadra: caso más conservador */
	}
}

void get_default_config(struct veml7700_dev *dev)
{
	dev->params.gain       = ALS_GAIN_1_8;
	dev->params.it         = ALS_IT_100MS;
	dev->params.pers       = ALS_PERS_1;
	dev->params.int_status = INT_DISABLED;
	dev->params.pwr_mode   = PWR_SAV_MODE_1;
	dev->params.res        = get_resolution(dev->params.gain, dev->params.it);
	dev->params.max_lux    = get_max_illumination(dev->params.gain, dev->params.it);
}

int32_t get_max_illumination(enum als_gain gain, enum als_it it_ms) {
	switch (it_ms) {
	case ALS_IT_800MS:
		switch (gain) {
		case ALS_GAIN_2:   return 275;
		case ALS_GAIN_1:   return 550;
		case ALS_GAIN_1_4: return 2202;
		case ALS_GAIN_1_8: return 4404;
		default:           return -1;
		}
		break;
	case ALS_IT_400MS:
		switch (gain) {
		case ALS_GAIN_2:   return 550;
		case ALS_GAIN_1:   return 1101;
		case ALS_GAIN_1_4: return 4404;
		case ALS_GAIN_1_8: return 8808;
		default:           return -1;
		}
		break;
	case ALS_IT_200MS:
		switch (gain) {
		case ALS_GAIN_2:   return 1101;
		case ALS_GAIN_1:   return 2202;
		case ALS_GAIN_1_4: return 8808;
		case ALS_GAIN_1_8: return 17616;
		default:           return -1;
		}
		break;
	case ALS_IT_100MS:
		switch (gain) {
		case ALS_GAIN_2:   return 2202;
		case ALS_GAIN_1:   return 4404;
		case ALS_GAIN_1_4: return 17616;
		case ALS_GAIN_1_8: return 35232;
		default:           return -1;
		}
		break;
	case ALS_IT_50MS:
		switch (gain) {
		case ALS_GAIN_2:   return 4404;
		case ALS_GAIN_1:   return 8808;
		case ALS_GAIN_1_4: return 35232;
		case ALS_GAIN_1_8: return 70463;
		default:           return -1;
		}
		break;
	case ALS_IT_25MS:
		switch (gain) {
		case ALS_GAIN_2:   return 8808;
		case ALS_GAIN_1:   return 17616;
		case ALS_GAIN_1_4: return 70463;
		case ALS_GAIN_1_8: return 140926;
		default:           return -1;
		}
		break;
	default:
		return -1;
	}
}

float get_resolution(enum als_gain gain, enum als_it it_ms)
{
	switch (it_ms) {
	case ALS_IT_800MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0042;
		case ALS_GAIN_1:   return 0.0084;
		case ALS_GAIN_1_4: return 0.0336;
		case ALS_GAIN_1_8: return 0.0672;
		default:           return -1;
		}
		break;
	case ALS_IT_400MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0084;
		case ALS_GAIN_1:   return 0.0168;
		case ALS_GAIN_1_4: return 0.0672;
		case ALS_GAIN_1_8: return 0.1344;
		default:           return -1;
		}
		break;
	case ALS_IT_200MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0168;
		case ALS_GAIN_1:   return 0.0336;
		case ALS_GAIN_1_4: return 0.1344;
		case ALS_GAIN_1_8: return 0.2688;
		default:           return -1;
		}
		break;
	case ALS_IT_100MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0336;
		case ALS_GAIN_1:   return 0.0672;
		case ALS_GAIN_1_4: return 0.2688;
		case ALS_GAIN_1_8: return 0.5376;
		default:           return -1;
		}
		break;
	case ALS_IT_50MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.0672;
		case ALS_GAIN_1:   return 0.1344;
		case ALS_GAIN_1_4: return 0.5376;
		case ALS_GAIN_1_8: return 1.0752;
		default:           return -1;
		}
		break;
	case ALS_IT_25MS:
		switch (gain) {
		case ALS_GAIN_2:   return 0.1344;
		case ALS_GAIN_1:   return 0.2688;
		case ALS_GAIN_1_4: return 1.0752;
		case ALS_GAIN_1_8: return 2.1504;
		default:           return -1;
		}
		break;
	default:
		return -1;
	}
}

esp_err_t set_and_send_config(struct veml7700_dev *dev)
{
	esp_err_t error;
	uint16_t reg_data = 0;

	reg_data = (
		(dev->params.gain       << 11) |
		(dev->params.it         <<  6) |
		(dev->params.pers       <<  4) |
		(dev->params.int_status <<  1)
	);

	dev->params.max_lux = get_max_illumination(dev->params.gain, dev->params.it);
	dev->params.res     = get_resolution(dev->params.gain, dev->params.it);

	error = write_reg(dev, CMD_ALS_CONFIG, reg_data);

	return error;
}

esp_err_t read_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16_t *v)
{
	esp_err_t        error;
	i2c_cmd_handle_t cmd;
	uint8_t          rx[2];

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg, true);

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
	i2c_master_read(cmd, rx, 2, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(RD_TOUT_MS));

	*v = (uint16_t) ((rx[1] << 8) | rx[0]);
	i2c_cmd_link_delete(cmd);

	return error;
}

esp_err_t write_reg(struct veml7700_dev *dev, enum cmd_code reg, uint16_t v)
{
	esp_err_t        error;
	i2c_cmd_handle_t cmd;
	uint8_t          tx[2];

	tx[0] = (uint8_t) v; /* LSB */
	tx[1] = (uint8_t) (v >> 8); /* MSB */

	cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, false);
	i2c_master_write_byte(cmd, reg, false);

	i2c_master_write(cmd, tx, 2, false);
	i2c_master_stop(cmd);

	error = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(RD_TOUT_MS));

	i2c_cmd_link_delete(cmd);

	return error;
}

inline void timer_restart(struct veml7700_dev *dev, uint32_t limit_ms)
{
	dev->start_ticks = xTaskGetTickCount();
	dev->limit_ticks = pdMS_TO_TICKS(limit_ms);
}

inline bool timer_elapsed(struct veml7700_dev *dev)
{
	bool res;

	res = (xTaskGetTickCount() - dev->start_ticks) >= dev->limit_ticks;

	return res;
}
