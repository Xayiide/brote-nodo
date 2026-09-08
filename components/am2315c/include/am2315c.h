#ifndef AM2315C_H_
#define AM2315C_H_

#include <stdint.h> /* uint */
#include <stdbool.h> /* bool */

#include <portmacro.h> /* TickType_t */
#include <esp_err.h> /* esp_err_t */

#define AM2315C_MAX_NUM_DEV 1

void       am2315c_init(void);
void       am2315c_add_dev(uint8_t addr, uint16_t period_ms);
esp_err_t  am2315c_get_hum(uint8_t addr, float *hum);
esp_err_t  am2315c_get_temp(uint8_t addr, float *temp);
esp_err_t  am2315c_poll(void);
TickType_t am2315c_get_min_wait(void);
esp_err_t  am2315c_get_dev_err(uint8_t addr);
bool       am2315c_is_sample_ready(uint8_t addr);


#endif /* AM2315C_H_ */
