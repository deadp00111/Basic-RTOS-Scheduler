#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdint.h>
#include <stddef.h>

/* USART1: PA9=TX, PA10=RX, AF7  -- interactive CLI, 115200 8N1, RX is interrupt-driven */
void BSP_UART1_Init(uint32_t baud);
void BSP_UART1_SendString(const char *s);
void BSP_UART1_SendBuf(const uint8_t *buf, size_t len);

/* USART2: PA2=TX, PA3=RX, AF7   -- log stream / secondary console, 115200 8N1, polled */
void BSP_UART2_Init(uint32_t baud);
void BSP_UART2_SendString(const char *s);

#endif
