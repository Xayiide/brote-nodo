#ifndef VEML7700_H_
#define VEML7700_H_

#include <stdint.h> /* uint */

#include <portmacro.h> /* TickType_t */
#include <esp_err.h> /* esp_err_t */

#define VEML7700_MAX_NUM_DEV 1

void       veml7700_init(void);
void       veml7700_add_dev(uint8_t addr, uint16_t period_ms);
esp_err_t  veml7700_get_lux(uint8_t addr, float *lx);
esp_err_t  veml7700_get_white(uint8_t addr, float *wh);
esp_err_t  veml7700_get_cfg(uint8_t addr, uint16_t *it, uint8_t *gain);
esp_err_t  veml7700_get_res(uint8_t addr, float *r);
esp_err_t  veml7700_get_raw(uint8_t addr, uint16_t *raw_lx, uint16_t *raw_wh);
esp_err_t  veml7700_read_all_devs(void);
TickType_t veml7700_get_min_wait(void);
esp_err_t  veml7700_get_dev_err(uint8_t addr);

#endif /* VEML7700_H_ */
