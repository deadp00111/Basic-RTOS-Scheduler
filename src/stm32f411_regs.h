
#ifndef STM32F411_REGS_H
#define STM32F411_REGS_H

#include <stdint.h>

#define __IO volatile

/* ---------------- Core peripherals (ARMv7-M standard, PM0214) --------- */

typedef struct {
    __IO uint32_t CTRL;   /* 0xE000E010 : control & status */
    __IO uint32_t LOAD;   /* 0xE000E014 : reload value      */
    __IO uint32_t VAL;    /* 0xE000E018 : current value     */
    __IO uint32_t CALIB;  /* 0xE000E01C : calibration        */
} SysTick_Type;

#define SysTick_BASE   (0xE000E010UL)
#define SysTick        ((SysTick_Type *)SysTick_BASE)

#define SysTick_CTRL_ENABLE_Msk    (1UL << 0)  /* counter enable       */
#define SysTick_CTRL_TICKINT_Msk   (1UL << 1)  /* exception request en */
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2)  /* 1 = processor clock  */
#define SysTick_CTRL_COUNTFLAG_Msk (1UL << 16)

/* System Control Block — only what we need to force a clean reset vector */
#define SCB_VTOR (*(__IO uint32_t *)0xE000ED08UL)

/* ---------------- RCC (Reset & Clock Control), base 0x40023800 -------- */

typedef struct {
    __IO uint32_t CR;          /* 0x00 */
    __IO uint32_t PLLCFGR;      /* 0x04 */
    __IO uint32_t CFGR;         /* 0x08 */
    __IO uint32_t CIR;          /* 0x0C */
    __IO uint32_t AHB1RSTR;     /* 0x10 */
    __IO uint32_t AHB2RSTR;     /* 0x14 */
    uint32_t RESERVED0[2];
    __IO uint32_t APB1RSTR;     /* 0x20 */
    __IO uint32_t APB2RSTR;     /* 0x24 */
    uint32_t RESERVED1[2];
    __IO uint32_t AHB1ENR;      /* 0x30 : peripheral clock enables (AHB1) */
    __IO uint32_t AHB2ENR;      /* 0x34 */
    uint32_t RESERVED2[2];
    __IO uint32_t APB1ENR;      /* 0x40 */
    __IO uint32_t APB2ENR;      /* 0x44 */
} RCC_TypeDef;

#define RCC_BASE          (0x40023800UL)
#define RCC                ((RCC_TypeDef *)RCC_BASE)
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)   /* bit0 = GPIOA clock enable */
#define RCC_AHB1ENR_GPIOCEN (1UL << 2)   /* bit2 = GPIOC clock enable */
#define RCC_APB2ENR_USART1EN (1UL << 4)  /* bit4 = USART1 clock enable */

/* ---------------- GPIO, base addresses for A and C --------------------- */

typedef struct {
    __IO uint32_t MODER;    /* 0x00 : mode register (2 bits/pin)        */
    __IO uint32_t OTYPER;   /* 0x04 : output type                       */
    __IO uint32_t OSPEEDR;  /* 0x08 : output speed                      */
    __IO uint32_t PUPDR;    /* 0x0C : pull-up/pull-down                 */
    __IO uint32_t IDR;      /* 0x10 : input data                        */
    __IO uint32_t ODR;      /* 0x14 : output data                       */
    __IO uint32_t BSRR;     /* 0x18 : bit set/reset (atomic)             */
    __IO uint32_t LCKR;     /* 0x1C : config lock                       */
    __IO uint32_t AFR[2];   /* 0x20 : alternate function low/high        */
} GPIO_TypeDef;

#define GPIOA_BASE   (0x40020000UL)
#define GPIOA        ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOC_BASE   (0x40020800UL)
#define GPIOC        ((GPIO_TypeDef *)GPIOC_BASE)

#define GPIO_MODER_OUTPUT  0x1u
#define GPIO_MODER_AF      0x2u
#define GPIO_PUPDR_PULLUP  0x1u
#define GPIO_AF7_USART1    0x7u

/* ---------------- USART1, base 0x40011000 ------------------------------ */

typedef struct {
    __IO uint32_t SR;    /* 0x00 : status register       */
    __IO uint32_t DR;    /* 0x04 : data register          */
    __IO uint32_t BRR;   /* 0x08 : baud rate register      */
    __IO uint32_t CR1;   /* 0x0C : control register 1       */
    __IO uint32_t CR2;   /* 0x10 : control register 2       */
    __IO uint32_t CR3;   /* 0x14 : control register 3       */
    __IO uint32_t GTPR;  /* 0x18 : guard time & prescaler   */
} USART_TypeDef;

#define USART1_BASE  (0x40011000UL)
#define USART1       ((USART_TypeDef *)USART1_BASE)

#define USART_SR_TXE   (1UL << 7)   /* transmit data register empty */
#define USART_CR1_UE   (1UL << 13)  /* USART enable                  */
#define USART_CR1_TE   (1UL << 3)   /* transmitter enable             */

#endif /* STM32F411_REGS_H */