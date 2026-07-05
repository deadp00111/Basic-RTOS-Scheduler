#ifndef BSP_SPI_SD_H
#define BSP_SPI_SD_H

#include <stdint.h>
#include <stddef.h>

/* SPI1: PA5=SCK, PA6=MISO, PA7=MOSI, PA4=CS (software-controlled), AF5.
 * Implements the SD card SPI-mode bring-up (CMD0/CMD8/ACMD41) and raw
 * 512-byte block writes. No filesystem layer - the logger task writes
 * sequential fixed-size records; layer FatFs on top if you need a real FS.
 * Callers must hold xSDMutex before calling any of these.
 */
int  BSP_SD_Init(void);                                  /* 0 = ok */
int  BSP_SD_WriteBlock(uint32_t lba, const uint8_t *buf512);
int  BSP_SD_ReadBlock(uint32_t lba, uint8_t *buf512);

#endif
