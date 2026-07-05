#include "bsp_spi_sd.h"
#include "main.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "task.h"

#define CS_LOW()   (GPIOA->BSRR = (1u << (4 + 16)))
#define CS_HIGH()  (GPIOA->BSRR = (1u << 4))

static int s_sdhc = 0;

static uint8_t spi_xfer(uint8_t out)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = out;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return (uint8_t)SPI1->DR;
}

static void spi_set_slow(void)  { SPI1->CR1 = (SPI1->CR1 & ~SPI_CR1_BR) | (7u << 3); } /* /256 for init */
static void spi_set_fast(void)  { SPI1->CR1 = (SPI1->CR1 & ~SPI_CR1_BR) | (1u << 3); } /* /4 for data   */

static void spi_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA5=SCK, PA6=MISO, PA7=MOSI -> AF5 */
    GPIOA->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOA->MODER |=  (2u << (5 * 2)) | (2u << (6 * 2)) | (2u << (7 * 2));
    GPIOA->OSPEEDR |= (3u << (5 * 2)) | (3u << (6 * 2)) | (3u << (7 * 2));
    GPIOA->AFR[0] &= ~(0xFu << (5 * 4) | 0xFu << (6 * 4) | 0xFu << (7 * 4));
    GPIOA->AFR[0] |=  (5u  << (5 * 4)) | (5u << (6 * 4)) | (5u << (7 * 4));

    /* PA4 = CS, manual GPIO output, idle high */
    GPIOA->MODER &= ~GPIO_MODER_MODER4;
    GPIOA->MODER |=  (1u << (4 * 2));
    CS_HIGH();

    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | (7u << 3); /* slow, mode 0, 8-bit */
    SPI1->CR1 |= SPI_CR1_SPE;
}

static uint8_t sd_cmd(uint8_t cmd_idx, uint32_t arg, uint8_t crc)
{
    spi_xfer(0x40 | cmd_idx);
    spi_xfer((uint8_t)(arg >> 24));
    spi_xfer((uint8_t)(arg >> 16));
    spi_xfer((uint8_t)(arg >> 8));
    spi_xfer((uint8_t)arg);
    spi_xfer(crc);

    uint8_t resp = 0xFF;
    for (int i = 0; i < 8; i++) {
        resp = spi_xfer(0xFF);
        if (!(resp & 0x80)) break; /* R1 valid once MSB clears */
    }
    return resp;
}

int BSP_SD_Init(void)
{
    spi_gpio_init();
    spi_set_slow();

    CS_HIGH();
    for (int i = 0; i < 10; i++) spi_xfer(0xFF); /* >=74 clocks with CS high */

    CS_LOW();
    if (sd_cmd(0, 0, 0x95) != 0x01) { CS_HIGH(); return -1; }              /* CMD0 -> idle */

    uint8_t r1 = sd_cmd(8, 0x1AA, 0x87);                                    /* CMD8 */
    if (r1 == 0x01) {
        for (int i = 0; i < 4; i++) spi_xfer(0xFF); /* discard R7 payload */
    }

    /* ACMD41 until card leaves idle (poll, bounded) */
    int ready = 0;
    for (int tries = 0; tries < 20000; tries++) {
        sd_cmd(55, 0, 0x65);                       /* CMD55 */
        uint8_t r = sd_cmd(41, 0x40000000, 0x77);   /* ACMD41, HCS=1 */
        if (r == 0x00) { ready = 1; break; }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!ready) { CS_HIGH(); return -2; }

    uint8_t ocr0 = sd_cmd(58, 0, 0xFD);            /* CMD58 -> OCR, check CCS bit */
    if (ocr0 == 0x00) {
        uint8_t b1 = spi_xfer(0xFF);
        s_sdhc = (b1 & 0x40) ? 1 : 0;
        spi_xfer(0xFF); spi_xfer(0xFF); spi_xfer(0xFF);
    }
    CS_HIGH();
    spi_xfer(0xFF);
    spi_set_fast();
    return 0;
}

static int wait_token(uint8_t want, int timeout_loops)
{
    uint8_t t;
    do {
        t = spi_xfer(0xFF);
    } while (t != want && --timeout_loops > 0);
    return (t == want) ? 0 : -1;
}

int BSP_SD_WriteBlock(uint32_t lba, const uint8_t *buf512)
{
    uint32_t addr = s_sdhc ? lba : (lba * 512u);
    CS_LOW();
    if (sd_cmd(24, addr, 0xFF) != 0x00) { CS_HIGH(); return -1; }   /* CMD24 */

    spi_xfer(0xFF);        /* one byte gap        */
    spi_xfer(0xFE);        /* data start token     */
    for (int i = 0; i < 512; i++) spi_xfer(buf512[i]);
    spi_xfer(0xFF); spi_xfer(0xFF); /* dummy CRC   */

    uint8_t resp = spi_xfer(0xFF);
    if ((resp & 0x1F) != 0x05) { CS_HIGH(); return -2; }   /* data rejected */

    if (wait_token(0xFF, 200000) != 0) { CS_HIGH(); return -3; } /* wait not-busy */
    CS_HIGH();
    spi_xfer(0xFF);
    return 0;
}

int BSP_SD_ReadBlock(uint32_t lba, uint8_t *buf512)
{
    uint32_t addr = s_sdhc ? lba : (lba * 512u);
    CS_LOW();
    if (sd_cmd(17, addr, 0xFF) != 0x00) { CS_HIGH(); return -1; }  /* CMD17 */

    if (wait_token(0xFE, 200000) != 0) { CS_HIGH(); return -2; }
    for (int i = 0; i < 512; i++) buf512[i] = spi_xfer(0xFF);
    spi_xfer(0xFF); spi_xfer(0xFF); /* CRC, discarded */

    CS_HIGH();
    spi_xfer(0xFF);
    return 0;
}
