#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

/* ---------------- Logging backend select ----------------
 * LOG_TARGET_UART : stream CSV lines out USART2 (default - works with zero extra hardware)
 * LOG_TARGET_SD   : write fixed-size records to raw SD blocks over SPI1 (bsp_spi_sd.c)
 * Both drivers are built; the logger task just calls Log_WriteLine() which
 * dispatches to whichever backend is selected here.
 */
#define LOG_TARGET_UART   1
#define LOG_TARGET_SD     2
#define LOG_TARGET        LOG_TARGET_UART

/* ---------------- Shared data structures ---------------- */

typedef struct {
    uint32_t timestamp_ms;
    float    temperature_c;
    float    humidity_pct;
    uint16_t seq;
} SensorSample_t;

typedef enum {
    LOGSTATE_STOPPED = 0,
    LOGSTATE_RUNNING
} LogState_t;

/* System-wide config, changed by CLI, read by other tasks - protected by xConfigMutex */
typedef struct {
    uint32_t   sample_period_ms;   /* how often sensor task samples          */
    LogState_t log_state;          /* logger enabled/disabled via CLI        */
    float      temp_alarm_c;       /* CLI-settable alarm threshold           */
} SystemConfig_t;

/* ---------------- Global RTOS handles (defined in main.c) ---------------- */

extern QueueHandle_t   xSensorToLoggerQ;   /* SensorSample_t, sensor -> logger  */
extern QueueHandle_t   xSensorToDisplayQ;  /* SensorSample_t, sensor -> display, len 1 (latest value only) */
extern QueueHandle_t   xCliRxCharQ;        /* uint8_t, USART1 RX ISR -> CLI task */

extern SemaphoreHandle_t xI2CMutex;        /* guards I2C1 bus: OLED + sensor      */
extern SemaphoreHandle_t xUartLogMutex;    /* guards USART2 TX: logger stream      */
extern SemaphoreHandle_t xUart1Mutex;      /* guards USART1 TX: CLI + monitor console */
extern SemaphoreHandle_t xConfigMutex;     /* guards g_SystemConfig                */
extern SemaphoreHandle_t xSDMutex;         /* guards SPI1 / SD card access          */

extern TimerHandle_t     xSampleTimer;     /* software timer, ticks sensor task    */

extern SystemConfig_t    g_SystemConfig;

/* Task priorities come from FreeRTOSConfig.h (TASK_PRIO_*) */

/* Task entry points */
void vSensorTask(void *pvParameters);
void vLoggerTask(void *pvParameters);
void vDisplayTask(void *pvParameters);
void vCliTask(void *pvParameters);
void vMonitorTask(void *pvParameters);

/* Sample timer callback (lives in main.c, notifies sensor task) */
void vSampleTimerCallback(TimerHandle_t xTimer);

#endif /* APP_CONFIG_H */
