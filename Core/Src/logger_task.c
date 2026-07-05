#include "app_config.h"
#include "bsp_uart.h"
#if (LOG_TARGET == LOG_TARGET_SD)
#include "bsp_spi_sd.h"
#endif
#include <stdio.h>
#include <string.h>

#if (LOG_TARGET == LOG_TARGET_SD)
static uint8_t  s_block[512];
static size_t   s_block_used = 0;
static uint32_t s_next_lba   = 1000; /* leave LBA 0..999 free for a boot sector / superblock */

static void sd_flush_block(void)
{
    if (s_block_used == 0) return;
    memset(&s_block[s_block_used], 0, sizeof(s_block) - s_block_used);
    if (xSemaphoreTake(xSDMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        BSP_SD_WriteBlock(s_next_lba++, s_block);
        xSemaphoreGive(xSDMutex);
    }
    s_block_used = 0;
}

static void sd_append_line(const char *line, size_t len)
{
    if (s_block_used + len + 1 > sizeof(s_block)) sd_flush_block();
    memcpy(&s_block[s_block_used], line, len);
    s_block_used += len;
    s_block[s_block_used++] = '\n';
}
#endif

static void Log_WriteLine(const char *line, size_t len)
{
#if (LOG_TARGET == LOG_TARGET_UART)
    (void)len;
    if (xSemaphoreTake(xUartLogMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        BSP_UART2_SendString(line);
        xSemaphoreGive(xUartLogMutex);
    }
#else
    sd_append_line(line, len);
#endif
}

void vLoggerTask(void *pvParameters)
{
    (void)pvParameters;
    SensorSample_t sample;
    char line[80];

    /* CSV header once at boot */
    Log_WriteLine("seq,timestamp_ms,temperature_c,humidity_pct\r\n", 44);

    for (;;) {
        if (xQueueReceive(xSensorToLoggerQ, &sample, portMAX_DELAY) == pdTRUE) {

            LogState_t state;
            xSemaphoreTake(xConfigMutex, portMAX_DELAY);
            state = g_SystemConfig.log_state;
            xSemaphoreGive(xConfigMutex);

            if (state != LOGSTATE_RUNNING) continue;   /* CLI can pause logging without stopping sampling */

            int n = snprintf(line, sizeof(line), "%u,%lu,%.2f,%.2f\r\n",
                              sample.seq,
                              (unsigned long)sample.timestamp_ms,
                              (double)sample.temperature_c,
                              (double)sample.humidity_pct);
            if (n > 0) Log_WriteLine(line, (size_t)n);
        }
    }
}
