#ifndef BSP_I2C_H
#define BSP_I2C_H

#include <stdint.h>
#include <stddef.h>

/* I2C1: PB6=SCL, PB7=SDA, AF4, 100kHz standard mode.
 * Blocking/polling driver - callers must hold xI2CMutex (see app_config.h)
 * before calling any of these, since the OLED and sensor share this bus.
 */
void     BSP_I2C1_Init(void);
int      BSP_I2C1_WriteReg(uint8_t dev_addr7, uint8_t reg, const uint8_t *data, size_t len);
int      BSP_I2C1_ReadReg(uint8_t dev_addr7, uint8_t reg, uint8_t *data, size_t len);
int      BSP_I2C1_WriteRaw(uint8_t dev_addr7, const uint8_t *data, size_t len);

#endif
