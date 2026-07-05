#include "app_config.h"
#include "bsp_uart.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void cli_print(const char *s)
{
    if (xSemaphoreTake(xUart1Mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        BSP_UART1_SendString(s);
        xSemaphoreGive(xUart1Mutex);
    }
}

static void cmd_help(void)
{
    cli_print("Commands:\r\n"
               "  HELP              - this text\r\n"
               "  STATUS            - current config + last-known state\r\n"
               "  LOG START|STOP    - enable/disable the logger task's writes\r\n"
               "  SET PERIOD <ms>   - change sensor sample period (100-60000)\r\n"
               "  SET ALARM <c>     - change temperature alarm threshold\r\n> ");
}

static void cmd_status(void)
{
    char buf[96];
    xSemaphoreTake(xConfigMutex, portMAX_DELAY);
    SystemConfig_t cfg = g_SystemConfig;
    xSemaphoreGive(xConfigMutex);

    snprintf(buf, sizeof(buf), "period=%lums log=%s alarm=%.1fC\r\n> ",
             (unsigned long)cfg.sample_period_ms,
             (cfg.log_state == LOGSTATE_RUNNING) ? "RUNNING" : "STOPPED",
             (double)cfg.temp_alarm_c);
    cli_print(buf);
}

static void cmd_log(char *arg)
{
    if (strcasecmp(arg, "START") == 0) {
        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        g_SystemConfig.log_state = LOGSTATE_RUNNING;
        xSemaphoreGive(xConfigMutex);
        cli_print("Logging started.\r\n> ");
    } else if (strcasecmp(arg, "STOP") == 0) {
        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        g_SystemConfig.log_state = LOGSTATE_STOPPED;
        xSemaphoreGive(xConfigMutex);
        cli_print("Logging stopped.\r\n> ");
    } else {
        cli_print("Usage: LOG START|STOP\r\n> ");
    }
}

static void cmd_set(char *rest)
{
    char what[16];
    float val;
    if (sscanf(rest, "%15s %f", what, &val) != 2) {
        cli_print("Usage: SET PERIOD <ms> | SET ALARM <c>\r\n> ");
        return;
    }

    if (strcasecmp(what, "PERIOD") == 0) {
        uint32_t ms = (uint32_t)val;
        if (ms < 100 || ms > 60000) { cli_print("period must be 100-60000 ms\r\n> "); return; }
        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        g_SystemConfig.sample_period_ms = ms;
        xSemaphoreGive(xConfigMutex);
        xTimerChangePeriod(xSampleTimer, pdMS_TO_TICKS(ms), pdMS_TO_TICKS(100));
        cli_print("Sample period updated.\r\n> ");
    } else if (strcasecmp(what, "ALARM") == 0) {
        xSemaphoreTake(xConfigMutex, portMAX_DELAY);
        g_SystemConfig.temp_alarm_c = val;
        xSemaphoreGive(xConfigMutex);
        cli_print("Alarm threshold updated.\r\n> ");
    } else {
        cli_print("Unknown SET target.\r\n> ");
    }
}

static void dispatch(char *line)
{
    /* split off the first word */
    char *sp = strchr(line, ' ');
    char *arg = "";
    if (sp) { *sp = '\0'; arg = sp + 1; }

    if (strcasecmp(line, "HELP") == 0)        cmd_help();
    else if (strcasecmp(line, "STATUS") == 0) cmd_status();
    else if (strcasecmp(line, "LOG") == 0)    cmd_log(arg);
    else if (strcasecmp(line, "SET") == 0)    cmd_set(arg);
    else if (strlen(line) == 0)               cli_print("> ");
    else                                       cli_print("Unknown command. Type HELP.\r\n> ");
}

void vCliTask(void *pvParameters)
{
    (void)pvParameters;
    char line[64];
    size_t idx = 0;
    uint8_t ch;

    for (;;) {
        if (xQueueReceive(xCliRxCharQ, &ch, portMAX_DELAY) == pdTRUE) {

            if (ch == '\r' || ch == '\n') {
                line[idx] = '\0';
                cli_print("\r\n");
                dispatch(line);
                idx = 0;
            } else if (ch == 0x08 || ch == 0x7F) {      /* backspace/DEL */
                if (idx > 0) { idx--; cli_print("\b \b"); }
            } else if (idx < sizeof(line) - 1) {
                line[idx++] = (char)ch;
                if (xSemaphoreTake(xUart1Mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                    BSP_UART1_SendBuf(&ch, 1);            /* local echo */
                    xSemaphoreGive(xUart1Mutex);
                }
            }
        }
    }
}
