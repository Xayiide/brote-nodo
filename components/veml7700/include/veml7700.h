#ifndef VEML7700_H_
#define VEML7700_H_

#include <stdint.h> /* uint */

void    veml7700_init();
void    veml7700_add_dev(uint8_t addr, uint16_t period_ms);
float   veml7700_lux(uint8_t addr);
float   veml7700_white(uint8_t addr);
uint8_t veml7700_get_cfg(uint8_t addr, uint16_t *it, uint8_t *gain);
uint8_t veml7700_get_res(uint8_t addr, float *r);

#endif /* VEML7700_H_ */
