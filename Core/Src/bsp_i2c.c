#include "bsp_i2c.h"
#include "main.h"
#include "stm32f4xx.h"

#define I2C_TIMEOUT_LOOPS   100000u

static int wait_flag_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t t = I2C_TIMEOUT_LOOPS;
    while (!(*reg & mask)) {
        if (--t == 0) return -1;
    }
    return 0;
}

void BSP_I2C1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6=SCL, PB7=SDA -> AF4, open-drain, pull-up, high speed */
    GPIOB->MODER   &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOB->MODER   |=  (2u << (6 * 2)) | (2u << (7 * 2));
    GPIOB->OTYPER  |=  (1u << 6) | (1u << 7);
    GPIOB->OSPEEDR |=  (3u << (6 * 2)) | (3u << (7 * 2));
    GPIOB->PUPDR   &= ~(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);
    GPIOB->PUPDR   |=  (1u << (6 * 2)) | (1u << (7 * 2));
    GPIOB->AFR[0]  &= ~(0xFu << (6 * 4) | 0xFu << (7 * 4));
    GPIOB->AFR[0]  |=  (4u   << (6 * 4)) | (4u << (7 * 4));

    I2C1->CR1 = I2C_CR1_SWRST;
    I2C1->CR1 = 0;

    /* PCLK1 in MHz for CR2.FREQ; 100kHz standard mode timing per RM0383 */
    uint32_t pclk1_mhz = PCLK1_HZ / 1000000u;
    I2C1->CR2  = pclk1_mhz;
    I2C1->CCR  = (PCLK1_HZ / (2u * 100000u));      /* Sm mode, 100kHz */
    I2C1->TRISE = pclk1_mhz + 1u;
    I2C1->CR1  |= I2C_CR1_PE;
}

static int i2c_start_addr(uint8_t dev_addr7, int is_read)
{
    I2C1->CR1 |= I2C_CR1_START;
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_SB) != 0) return -1;

    I2C1->DR = (uint8_t)((dev_addr7 << 1) | (is_read ? 1u : 0u));
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_ADDR) != 0) return -1;
    (void)I2C1->SR2; /* clear ADDR by reading SR1 then SR2 */
    return 0;
}

int BSP_I2C1_WriteReg(uint8_t dev_addr7, uint8_t reg, const uint8_t *data, size_t len)
{
    if (i2c_start_addr(dev_addr7, 0) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }

    if (wait_flag_set(&I2C1->SR1, I2C_SR1_TXE) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
    I2C1->DR = reg;

    for (size_t i = 0; i < len; i++) {
        if (wait_flag_set(&I2C1->SR1, I2C_SR1_TXE) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
        I2C1->DR = data[i];
    }
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_BTF) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

int BSP_I2C1_WriteRaw(uint8_t dev_addr7, const uint8_t *data, size_t len)
{
    if (i2c_start_addr(dev_addr7, 0) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }

    for (size_t i = 0; i < len; i++) {
        if (wait_flag_set(&I2C1->SR1, I2C_SR1_TXE) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
        I2C1->DR = data[i];
    }
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_BTF) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}

int BSP_I2C1_ReadReg(uint8_t dev_addr7, uint8_t reg, uint8_t *data, size_t len)
{
    /* Write the register pointer first (no STOP - restart into a read) */
    if (i2c_start_addr(dev_addr7, 0) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_TXE) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
    I2C1->DR = reg;
    if (wait_flag_set(&I2C1->SR1, I2C_SR1_BTF) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }

    I2C1->CR1 |= I2C_CR1_ACK;
    if (i2c_start_addr(dev_addr7, 1) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }

    for (size_t i = 0; i < len; i++) {
        if (i == len - 1) I2C1->CR1 &= ~I2C_CR1_ACK; /* NACK the last byte */
        if (wait_flag_set(&I2C1->SR1, I2C_SR1_RXNE) != 0) { I2C1->CR1 |= I2C_CR1_STOP; return -1; }
        data[i] = (uint8_t)I2C1->DR;
    }
    I2C1->CR1 |= I2C_CR1_STOP;
    return 0;
}
