#include "app_config.h"
#include "bsp_i2c.h"
#include <math.h>

/* Generic I2C sensor placeholder, e.g. SHT31-class temp/humidity sensor.
 * Adjust SENSOR_I2C_ADDR and the trigger/read sequence for your actual part
 * (BME280, SHT31, HDC1080, ...) - the RTOS scaffolding around it doesn't change.
 */
#define SENSOR_I2C_ADDR   0x44
#define SENSOR_CMD_MEAS_MSB 0x2C
#define SENSOR_CMD_MEAS_LSB 0x06

static bool sensor_read_raw(float *temp_c, float *hum_pct)
{
    uint8_t cmd[1] = { SENSOR_CMD_MEAS_LSB };
    uint8_t raw[6];

    if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return false; /* bus busy with the OLED - skip this cycle rather than block forever */
    }

    int ok = (BSP_I2C1_WriteReg(SENSOR_I2C_ADDR, SENSOR_CMD_MEAS_MSB, cmd, 1) == 0);
    if (ok) {
        /* real sensors need a few ms conversion delay here; omitted for brevity */
        ok = (BSP_I2C1_ReadReg(SENSOR_I2C_ADDR, 0x00, raw, 6) == 0);
    }
    xSemaphoreGive(xI2CMutex);

    if (!ok) return false;

    uint16_t raw_t = (raw[0] << 8) | raw[1];
    uint16_t raw_h = (raw[3] << 8) | raw[4];
    *temp_c  = -45.0f + 175.0f * ((float)raw_t / 65535.0f);
    *hum_pct = 100.0f * ((float)raw_h / 65535.0f);
    return true;
}

/* Deterministic fallback so the whole pipeline (queues/logger/display/CLI)
 * is demonstrable on a bench without the physical sensor wired up. */
static void sensor_simulate(uint32_t seq, float *temp_c, float *hum_pct)
{
    *temp_c  = 24.0f + 3.0f * sinf((float)seq * 0.10f);
    *hum_pct = 45.0f + 5.0f * cosf((float)seq * 0.07f);
}

void vSensorTask(void *pvParameters)
{
    (void)pvParameters;
    uint16_t seq = 0;

    for (;;) {
        /* Blocks here until vSampleTimerCallback() gives a notification -
         * decouples "how often we sample" (CLI/timer-controlled) from the
         * task body, and avoids a second vTaskDelay-based timing source. */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        SensorSample_t sample;
        sample.seq = seq++;
        sample.timestamp_ms = xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);

        if (!sensor_read_raw(&sample.temperature_c, &sample.humidity_pct)) {
            sensor_simulate(sample.seq, &sample.temperature_c, &sample.humidity_pct);
        }

        /* Logger gets every sample (queued, may briefly block the producer if full) */
        xQueueSend(xSensorToLoggerQ, &sample, pdMS_TO_TICKS(20));

        /* Display only ever needs the latest value - overwrite instead of queueing */
        xQueueOverwrite(xSensorToDisplayQ, &sample);
    }
}
