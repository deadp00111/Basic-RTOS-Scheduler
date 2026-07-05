#include "main.h"
#include "stm32f4xx.h"
#include "app_config.h"
#include "bsp_uart.h"
#include "bsp_i2c.h"
#include "bsp_spi_sd.h"
#include "ssd1306.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ---------------- RTOS objects (declared extern in app_config.h) ---------------- */
QueueHandle_t     xSensorToLoggerQ;
QueueHandle_t     xSensorToDisplayQ;
QueueHandle_t     xCliRxCharQ;

SemaphoreHandle_t xI2CMutex;
SemaphoreHandle_t xUartLogMutex;
SemaphoreHandle_t xUart1Mutex;
SemaphoreHandle_t xConfigMutex;
SemaphoreHandle_t xSDMutex;

TimerHandle_t      xSampleTimer;

SystemConfig_t     g_SystemConfig = {
    .sample_period_ms = 500,
    .log_state        = LOGSTATE_RUNNING,
    .temp_alarm_c     = 40.0f
};

/* Notified by the software timer; consumed by vSensorTask to pace sampling
 * without letting the task itself own timing (keeps the "when to sample"
 * policy in one place, changeable at runtime via the CLI). */
static TaskHandle_t s_sensorTaskHandle;

void vSampleTimerCallback(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (s_sensorTaskHandle) {
        xTaskNotifyGive(s_sensorTaskHandle);
    }
}

/* SysTick is claimed by the FreeRTOS port (xPortSysTickHandler alias) once
 * vTaskStartScheduler() runs; before that we use it for the startup delay. */
static void delay_ms_blocking(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * (SYSCLK_HZ / 8000u); i++) { __NOP(); }
}

void SystemClock_Config(void)
{
    /* HSI(16MHz) -> /PLLM=8 -> 2MHz -> x PLLN=192 -> 384MHz VCO -> /PLLP=4 -> 96MHz SYSCLK
     * AHB=SYSCLK/1=96MHz, APB1=AHB/2=48MHz, APB2=AHB/1=96MHz
     */
    RCC->CR |= RCC_CR_HSION;
    while (!(RCC->CR & RCC_CR_HSIRDY)) { }

    FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN | (3u << FLASH_ACR_LATENCY_Pos);

    RCC->PLLCFGR = (8u << RCC_PLLCFGR_PLLM_Pos) |
                   (192u << RCC_PLLCFGR_PLLN_Pos) |
                   (1u << RCC_PLLCFGR_PLLP_Pos)  |   /* 01 = /4 */
                   (0u << RCC_PLLCFGR_PLLSRC_Pos);   /* HSI as PLL source */

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) { }

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) { }
}

void GPIO_Init_Onboard(void)
{
    /* PC13 = onboard LED on most STM32F411 "BlackPill"-style boards, active low */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    GPIOC->MODER &= ~GPIO_MODER_MODER13;
    GPIOC->MODER |=  (1u << (13 * 2));
    GPIOC->BSRR = (1u << 13); /* off */
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    SystemClock_Config();
    GPIO_Init_Onboard();

    BSP_UART1_Init(115200);   /* CLI console               */
    BSP_UART2_Init(115200);   /* log stream (LOG_TARGET_UART) */
    BSP_I2C1_Init();          /* shared bus: OLED + sensor  */

#if (LOG_TARGET == LOG_TARGET_SD)
    BSP_SD_Init();
#endif

    delay_ms_blocking(50);    /* let the OLED power rail settle */
    SSD1306_Init();

    BSP_UART1_SendString("\r\n=== RTOS Industrial Data Logger (STM32F411CEU6) ===\r\n");
    BSP_UART1_SendString("Type HELP for CLI commands.\r\n> ");

    /* ---- Queues ---- */
    xSensorToLoggerQ  = xQueueCreate(8, sizeof(SensorSample_t));
    xSensorToDisplayQ = xQueueCreate(1, sizeof(SensorSample_t));   /* only the latest value matters */
    xCliRxCharQ       = xQueueCreate(64, sizeof(uint8_t));

    /* ---- Mutexes (all real priority-inheriting mutexes, not plain semaphores) ---- */
    xI2CMutex     = xSemaphoreCreateMutex();
    xUartLogMutex = xSemaphoreCreateMutex();
    xUart1Mutex   = xSemaphoreCreateMutex();
    xConfigMutex  = xSemaphoreCreateMutex();
    xSDMutex      = xSemaphoreCreateMutex();

    configASSERT(xSensorToLoggerQ && xSensorToDisplayQ && xCliRxCharQ);
    configASSERT(xI2CMutex && xUartLogMutex && xUart1Mutex && xConfigMutex && xSDMutex);

    /* ---- Software timer: paces the sensor task; period is CLI-adjustable ---- */
    xSampleTimer = xTimerCreate("SampleTmr",
                                 pdMS_TO_TICKS(g_SystemConfig.sample_period_ms),
                                 pdTRUE, NULL, vSampleTimerCallback);
    configASSERT(xSampleTimer);

    /* ---- Tasks ---- */
    xTaskCreate(vSensorTask,  "Sensor",  256, NULL, TASK_PRIO_SENSOR,  &s_sensorTaskHandle);
    xTaskCreate(vLoggerTask,  "Logger",  256, NULL, TASK_PRIO_LOGGER,  NULL);
    xTaskCreate(vDisplayTask, "Display", 256, NULL, TASK_PRIO_DISPLAY, NULL);
    xTaskCreate(vCliTask,     "CLI",     384, NULL, TASK_PRIO_CLI,     NULL);
    xTaskCreate(vMonitorTask, "Monitor", 192, NULL, TASK_PRIO_MONITOR, NULL);

    xTimerStart(xSampleTimer, 0);

    vTaskStartScheduler();

    /* Only reached if the scheduler couldn't start (e.g. heap exhausted) */
    for (;;) { }
}

/* ---------------- FreeRTOS hooks ---------------- */

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask; (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) { }
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize)
{
    (void)ppxIdleTaskTCBBuffer; (void)ppxIdleTaskStackBuffer; (void)pulIdleTaskStackSize;
    /* Not used: configSUPPORT_STATIC_ALLOCATION is 0 (heap_4 dynamic allocation only). */
}
