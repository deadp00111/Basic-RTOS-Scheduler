#include "app_config.h"
#include "bsp_uart.h"
#include <stdio.h>

#define MONITOR_PERIOD_MS   5000

static void mon_print(const char *s)
{
    if (xSemaphoreTake(xUart1Mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        BSP_UART1_SendString(s);
        xSemaphoreGive(xUart1Mutex);
    }
}

void vMonitorTask(void *pvParameters)
{
    (void)pvParameters;
    TickType_t last_wake = xTaskGetTickCount();
    SensorSample_t sample = { 0 };
    char buf[128];

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(MONITOR_PERIOD_MS));

        size_t free_heap = xPortGetFreeHeapSize();
        UBaseType_t hwm  = uxTaskGetStackHighWaterMark(NULL);

        xQueuePeek(xSensorToDisplayQ, &sample, 0);

        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        float alarm = g_SystemConfig.temp_alarm_c;
        xSemaphoreGive(xConfigMutex);

        snprintf(buf, sizeof(buf), "\r\n[monitor] heap_free=%uB mon_stack_hwm=%uw temp=%.1fC\r\n> ",
                 (unsigned)free_heap, (unsigned)hwm, (double)sample.temperature_c);
        mon_print(buf);

        if (sample.temperature_c > alarm) {
            snprintf(buf, sizeof(buf), "\r\n[ALARM] temperature %.1fC exceeds threshold %.1fC\r\n> ",
                     (double)sample.temperature_c, (double)alarm);
            mon_print(buf);
        }
    }
}
