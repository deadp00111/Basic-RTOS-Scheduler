#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx.h"

/* Resulting bus frequencies from SystemClock_Config() in main.c:
 *   HSI 16MHz -> PLL -> SYSCLK 96MHz, AHB 96MHz, APB1 48MHz, APB2 96MHz
 * Used by bsp_uart.c / bsp_i2c.c for baud-rate / timing register math.
 */
#define SYSCLK_HZ   96000000UL
#define PCLK1_HZ    48000000UL
#define PCLK2_HZ    96000000UL

void SystemClock_Config(void);
void GPIO_Init_Onboard(void);   /* status LED etc. */

#endif
