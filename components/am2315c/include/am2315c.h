#ifndef AM2315C_H_
#define AM2315C_H_

#include <stdint.h> /* uint */
#include <portmacro.h> /* TickType_t */

void       am2315c_init(void);
void       am2315c_add_dev(uint8_t addr, uint16_t period_ms);
float      am2315c_hum(uint8_t addr);
float      am2315c_temp(uint8_t addr);
void       am2315c_read_all_devs(void);
TickType_t am2315c_get_min_wait(void);


#endif /* AM2315C_H_ */
