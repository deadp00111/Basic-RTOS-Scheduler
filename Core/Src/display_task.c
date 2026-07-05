#include "app_config.h"
#include "ssd1306.h"
#include <stdio.h>

#define DISPLAY_REFRESH_MS   400

void vDisplayTask(void *pvParameters)
{
    (void)pvParameters;
    SensorSample_t sample = { 0 };
    char line[24];
    TickType_t last_wake = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(DISPLAY_REFRESH_MS));

        /* Peek (don't consume) - the queue holds only "latest value", sensor
         * task owns writing it via xQueueOverwrite(). */
        xQueuePeek(xSensorToDisplayQ, &sample, 0);

        LogState_t log_state;
        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        log_state = g_SystemConfig.log_state;
        xSemaphoreGive(xConfigMutex);

        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            SSD1306_Clear();

            SSD1306_SetCursor(0, 0);
            snprintf(line, sizeof(line), "T:%5.1fC H:%4.1f%%", (double)sample.temperature_c, (double)sample.humidity_pct);
            SSD1306_WriteString(line);

            SSD1306_SetCursor(0, 1);
            snprintf(line, sizeof(line), "SEQ:%-5u LOG:%s", sample.seq, (log_state == LOGSTATE_RUNNING) ? "ON " : "OFF");
            SSD1306_WriteString(line);

            SSD1306_Flush();
            xSemaphoreGive(xI2CMutex);
        }
    }
}
