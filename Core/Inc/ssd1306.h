#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>

#define SSD1306_I2C_ADDR   0x3C   /* 7-bit address, common default */
#define SSD1306_WIDTH      128
#define SSD1306_HEIGHT     32     /* also common: 64 - adjust if your module differs */

void SSD1306_Init(void);
void SSD1306_Clear(void);
void SSD1306_SetCursor(uint8_t col, uint8_t row);   /* row = 8px text line, 0..(HEIGHT/8 - 1) */
void SSD1306_WriteString(const char *s);
void SSD1306_Flush(void);   /* pushes the local framebuffer out over I2C */

#endif
