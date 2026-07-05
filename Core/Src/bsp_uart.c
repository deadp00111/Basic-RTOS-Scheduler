#include "bsp_uart.h"
#include "main.h"
#include "stm32f4xx.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "queue.h"

/* Compute BRR for OVER8=0 : USARTDIV = fCK / (16 * baud), BRR = mantissa<<4 | frac(4b) */
static uint32_t usart_brr(uint32_t pclk_hz, uint32_t baud)
{
    uint32_t div100 = (pclk_hz * 25u) / (4u * baud); /* USARTDIV*100 */
    uint32_t mantissa = div100 / 100u;
    uint32_t frac100  = div100 - (mantissa * 100u);
    uint32_t frac16   = ((frac100 * 16u) + 50u) / 100u;
    if (frac16 >= 16u) { mantissa++; frac16 = 0u; }
    return (mantissa << 4) | frac16;
}

void BSP_UART1_Init(uint32_t baud)
{
    /* Clocks: GPIOA + USART1 (APB2) */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    /* PA9 = TX, PA10 = RX, AF7, push-pull, high speed */
    GPIOA->MODER   &= ~(GPIO_MODER_MODER9 | GPIO_MODER_MODER10);
    GPIOA->MODER   |=  (2u << (9 * 2)) | (2u << (10 * 2));      /* AF mode */
    GPIOA->OSPEEDR |=  (3u << (9 * 2)) | (3u << (10 * 2));
    GPIOA->AFR[1]  &= ~(0xFu << ((9 - 8) * 4) | 0xFu << ((10 - 8) * 4));
    GPIOA->AFR[1]  |=  (7u   << ((9 - 8) * 4)) | (7u << ((10 - 8) * 4));

    USART1->CR1 = 0;
    USART1->BRR = usart_brr(PCLK2_HZ, baud);
    USART1->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;

    NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    NVIC_EnableIRQ(USART1_IRQn);
}

void BSP_UART1_SendString(const char *s)
{
    while (*s) {
        while (!(USART1->SR & USART_SR_TXE)) { }
        USART1->DR = (uint8_t)*s++;
    }
}

void BSP_UART1_SendBuf(const uint8_t *buf, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        while (!(USART1->SR & USART_SR_TXE)) { }
        USART1->DR = buf[i];
    }
}

void BSP_UART2_Init(uint32_t baud)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2 = TX, PA3 = RX, AF7 */
    GPIOA->MODER   &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER   |=  (2u << (2 * 2)) | (2u << (3 * 2));
    GPIOA->OSPEEDR |=  (3u << (2 * 2)) | (3u << (3 * 2));
    GPIOA->AFR[0]  &= ~(0xFu << (2 * 4) | 0xFu << (3 * 4));
    GPIOA->AFR[0]  |=  (7u   << (2 * 4)) | (7u << (3 * 4));

    USART2->CR1 = 0;
    USART2->BRR = usart_brr(PCLK1_HZ, baud);
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void BSP_UART2_SendString(const char *s)
{
    while (*s) {
        while (!(USART2->SR & USART_SR_TXE)) { }
        USART2->DR = (uint8_t)*s++;
    }
}

/* --- ISR: USART1 RX --------------------------------------------------
 * Runs at priority configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY+1 (numerically
 * lower urgency than the kernel's syscall ceiling is NOT required here since
 * we only call FromISR APIs, which is safe at/below configMAX_SYSCALL_INTERRUPT_PRIORITY).
 * Pushes each received byte into xCliRxCharQ for the CLI task to consume.
 */
void USART1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (USART1->SR & USART_SR_RXNE) {
        uint8_t ch = (uint8_t)USART1->DR;
        xQueueSendFromISR(xCliRxCharQ, &ch, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
