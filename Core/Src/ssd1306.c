#include "ssd1306.h"
#include "bsp_i2c.h"
#include "font5x7.h"
#include <string.h>

#define OLED_CMD   0x00
#define OLED_DATA  0x40

static uint8_t s_fb[SSD1306_WIDTH * (SSD1306_HEIGHT / 8)];
static uint8_t s_col, s_page;

static void cmd(uint8_t c)
{
    uint8_t buf[2] = { OLED_CMD, c };
    BSP_I2C1_WriteRaw(SSD1306_I2C_ADDR, buf, 2);
}

void SSD1306_Init(void)
{
    static const uint8_t init_seq[] = {
        0xAE,             /* display off            */
        0xD5, 0x80,       /* clock div               */
        0xA8, SSD1306_HEIGHT - 1, /* multiplex ratio  */
        0xD3, 0x00,       /* display offset          */
        0x40,             /* start line = 0          */
        0x8D, 0x14,       /* charge pump on          */
        0x20, 0x00,       /* memory addr mode = horizontal */
        0xA1,             /* segment remap           */
        0xC8,             /* COM scan dir remapped   */
        0xDA, (SSD1306_HEIGHT == 32) ? 0x02 : 0x12, /* COM pins config */
        0x81, 0x8F,       /* contrast                */
        0xD9, 0xF1,       /* precharge               */
        0xDB, 0x40,       /* VCOM deselect           */
        0xA4,             /* entire display from RAM */
        0xA6,             /* normal (not inverted)   */
        0xAF              /* display ON              */
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) cmd(init_seq[i]);
    SSD1306_Clear();
    SSD1306_Flush();
}

void SSD1306_Clear(void)
{
    memset(s_fb, 0, sizeof(s_fb));
    s_col = 0; s_page = 0;
}

void SSD1306_SetCursor(uint8_t col, uint8_t row)
{
    s_col  = col * 6;   /* 5px glyph + 1px spacing */
    s_page = row;
}

void SSD1306_WriteString(const char *s)
{
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        if (s_col + 5 >= SSD1306_WIDTH) break;
        for (int i = 0; i < 5; i++) {
            s_fb[s_page * SSD1306_WIDTH + s_col + i] = font5x7[c][i];
        }
        s_col += 6;
    }
}

void SSD1306_Flush(void)
{
    for (uint8_t page = 0; page < SSD1306_HEIGHT / 8; page++) {
        cmd(0xB0 | page);   /* set page address    */
        cmd(0x00);          /* lower column = 0     */
        cmd(0x10);          /* upper column = 0     */

        uint8_t buf[SSD1306_WIDTH + 1];
        buf[0] = OLED_DATA;
        memcpy(&buf[1], &s_fb[page * SSD1306_WIDTH], SSD1306_WIDTH);
        BSP_I2C1_WriteRaw(SSD1306_I2C_ADDR, buf, sizeof(buf));
    }
}
