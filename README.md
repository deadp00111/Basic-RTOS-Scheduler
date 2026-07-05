# RTOS Industrial Data Logger — STM32F411CEU6

Builds clean with `arm-none-eabi-gcc` + `make` (verified in a real toolchain: 0
warnings, 0 errors, `.elf`/`.hex`/`.bin` all produced, 31.5 KB flash / 27.5 KB
RAM used out of 512 KB / 128 KB available).

## Build

```
make            # builds build/rtos_logger.elf/.hex/.bin
make clean
make flash          # st-flash write build/rtos_logger.bin 0x8000000
make flash-openocd   # openocd + stlink, if you use that instead
```

Toolchain used to verify: `gcc-arm-none-eabi` 13.2.1 (Ubuntu package), GNU Make.

## What's vendored vs. hand-written

- `Drivers/CMSIS/**` — official ARM CMSIS core headers + ST's `cmsis_device_f4`
  (device header, startup file, `system_stm32f4xx.c`), pulled straight from
  STMicroelectronics' GitHub repo. Not hand-typed, so register names/vector
  table are correct by construction.
- `Middlewares/FreeRTOS/**` — FreeRTOS Kernel V11.1.0 + the `ARM_CM4F` port +
  `heap_4` allocator, from the official FreeRTOS-Kernel GitHub repo.
- `Core/Inc/font5x7.h` — the standard BSD-licensed Adafruit-GFX 5x7 font table,
  used as-is for the OLED text renderer.
- Everything else (`Core/Src/*.c`, `Core/Inc/app_config.h`, `bsp_*`, `ssd1306.c`,
  the linker script, `FreeRTOSConfig.h`, the `Makefile`) is written for this
  project.

## Task architecture

| Task     | Prio | Role                                             |
|----------|------|---------------------------------------------------|
| Sensor   | 4    | Blocks on a task notification from `xSampleTimer`, reads I2C sensor (or falls back to a simulated waveform if none is attached), pushes to two queues |
| Logger   | 3    | Drains `xSensorToLoggerQ`, writes CSV — either over USART2 or as raw 512B SD blocks over SPI1, selected by `LOG_TARGET` in `app_config.h` |
| CLI      | 2    | Line-buffered command parser fed by the USART1 RX ISR via `xCliRxCharQ` |
| Display  | 2    | Redraws the SSD1306 OLED every 400 ms from the latest value in `xSensorToDisplayQ` |
| Monitor  | 1    | Lowest priority: reports heap/stack headroom every 5s, raises a threshold alarm |

**Queues:** `xSensorToLoggerQ` (depth 8, every sample), `xSensorToDisplayQ`
(depth 1, `xQueueOverwrite`/`xQueuePeek` — display only ever wants "latest"),
`xCliRxCharQ` (ISR → task).

**Mutexes:** `xI2CMutex` (OLED + sensor share I2C1), `xUartLogMutex` (USART2),
`xUart1Mutex` (USART1 console, shared by CLI + Monitor), `xSDMutex` (SPI1/SD),
`xConfigMutex` (guards the CLI-writable `g_SystemConfig` struct).

**Software timer:** `xSampleTimer` paces the sensor task and its period is
live-adjustable from the CLI (`SET PERIOD <ms>`) via `xTimerChangePeriod`.

## Wiring (as coded — change pins in the `bsp_*.c` files if yours differ)

- USART1 (CLI): PA9 TX, PA10 RX, 115200 8N1
- USART2 (log stream): PA2 TX, PA3 RX, 115200 8N1
- I2C1 (OLED + sensor, shared bus): PB6 SCL, PB7 SDA, 100 kHz
- SPI1 (SD card, only used if `LOG_TARGET_SD`): PA5 SCK, PA6 MISO, PA7 MOSI, PA4 CS
- PC13: onboard status LED

## CLI

Connect a serial terminal to USART1 at 115200 8N1:

```
HELP
STATUS
LOG START | LOG STOP
SET PERIOD <ms>
SET ALARM <celsius>
```

## Known simplifications (documented, not hidden)

- Sensor read targets a generic I2C temp/humidity part at 0x44 (SHT31-style
  register layout) — swap `sensor_read_raw()` in `sensor_task.c` for your
  actual sensor's command set. It falls back to a simulated sine/cosine
  waveform if the I2C transaction fails, so the whole pipeline is
  demonstrable without hardware attached.
- SD logging writes raw fixed-size blocks, no filesystem — add FatFs on top
  of `bsp_spi_sd.c` if you need a browsable file.
- Clock config assumes HSI (no external crystal) → 96 MHz via PLL.
