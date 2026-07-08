#include <stdint.h>
#include "scheduler.h"
#include "stm32f411_regs.h"

#define LED_PIN    13u
#define BTN_PIN    0u

static void led_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~(0x3u << (LED_PIN * 2));
    GPIOC->MODER |=  (GPIO_MODER_OUTPUT << (LED_PIN * 2));
}

static void led_on(void)  { GPIOC->BSRR = (1u << (LED_PIN + 16)); }
static void led_off(void) { GPIOC->BSRR = (1u << LED_PIN); }

static void button_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    GPIOA->MODER &= ~(0x3u << (BTN_PIN * 2));
    GPIOA->PUPDR &= ~(0x3u << (BTN_PIN * 2));
    GPIOA->PUPDR |=  (GPIO_PUPDR_PULLUP << (BTN_PIN * 2));
}

static void uart_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER &= ~((0x3u << (9 * 2)) | (0x3u << (10 * 2)));
    GPIOA->MODER |=  ((GPIO_MODER_AF << (9 * 2)) | (GPIO_MODER_AF << (10 * 2)));
    GPIOA->AFR[1] &= ~((0xFu << ((9 - 8) * 4)) | (0xFu << ((10 - 8) * 4)));
    GPIOA->AFR[1] |=  ((GPIO_AF7_USART1 << ((9 - 8) * 4)) | (GPIO_AF7_USART1 << ((10 - 8) * 4)));

    USART1->BRR = 0x8Bu;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE;
}

static void uart_write_char(char c)
{
    while (!(USART1->SR & USART_SR_TXE)) { }
    USART1->DR = (uint32_t)c;
}

static void uart_write_string(const char *s)
{
    while (*s) {
        uart_write_char(*s++);
    }
}

static void uart_write_uint(uint32_t value)
{
    char buf[11];
    int i = 10;
    buf[i] = '\0';

    if (value == 0) {
        uart_write_char('0');
        return;
    }
    while (value > 0 && i > 0) {
        buf[--i] = (char)('0' + (value % 10u));
        value /= 10u;
    }
    uart_write_string(&buf[i]);
}

static void systick_init_1ms(void)
{
    SysTick->LOAD = (16000000u / 1000u) - 1u;
    SysTick->VAL  = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk
                  | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    scheduler_tick_handler();
}

static volatile uint8_t blink_enabled = 1u;

typedef enum { LED_STATE_OFF = 0, LED_STATE_ON } led_state_t;

static void task_led_blink(void)
{
    static led_state_t state = LED_STATE_OFF;

    if (!blink_enabled) {
        led_off();
        state = LED_STATE_OFF;
        return;
    }

    switch (state) {
        case LED_STATE_OFF:
            led_on();
            state = LED_STATE_ON;
            break;
        case LED_STATE_ON:
            led_off();
            state = LED_STATE_OFF;
            break;
    }
}

typedef enum { BTN_IDLE = 0, BTN_CONFIRM, BTN_HELD } button_state_t;

static void task_button_poll(void)
{
    static button_state_t state = BTN_IDLE;
    uint8_t pressed = ((GPIOA->IDR & (1u << BTN_PIN)) == 0u);

    switch (state) {
        case BTN_IDLE:
            if (pressed) {
                state = BTN_CONFIRM;
            }
            break;

        case BTN_CONFIRM:
            if (pressed) {
                blink_enabled = !blink_enabled;
                uart_write_string("Button pressed, blink_enabled=");
                uart_write_uint(blink_enabled);
                uart_write_string("\r\n");
                state = BTN_HELD;
            } else {
                state = BTN_IDLE;
            }
            break;

        case BTN_HELD:
            if (!pressed) {
                state = BTN_IDLE;
            }
            break;
    }
}

static void task_uart_heartbeat(void)
{
    uart_write_string("tick=");
    uart_write_uint(scheduler_get_tick());
    uart_write_string(" ms, blink_enabled=");
    uart_write_uint(blink_enabled);
    uart_write_string("\r\n");
}

int main(void)
{
    led_gpio_init();
    button_gpio_init();
    uart_init();
    scheduler_init();

    scheduler_add_task(task_led_blink,      500u,  "led_blink");
    scheduler_add_task(task_button_poll,     20u,   "button_poll");
    scheduler_add_task(task_uart_heartbeat,  1000u, "uart_heartbeat");

    systick_init_1ms();

    uart_write_string("\r\n--- scheduler booted ---\r\n");

    while (1) {
        scheduler_run();
        __asm volatile ("wfi");
    }
}