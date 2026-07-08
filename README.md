# Cooperative Task Scheduler — STM32F411 (WeAct BlackPill)

A lightweight, from-scratch cooperative scheduler built around the ARM
SysTick timer, a function-pointer task table, and per-task state
machines. No RTOS, no HAL — plain register access.

## How it works

```
SysTick fires every 1ms
        |
        v
SysTick_Handler (ISR)
        |
        v
scheduler_tick_handler()   -> just counts down each task's delay timer
        |                      and flags it READY when the timer hits 0
        v
   (back to main loop)
        |
        v
   scheduler_run()          -> runs continuously in main()'s while(1);
        |                      dispatches any READY task through its
        v                      function pointer, then re-arms its period
  task_led_blink() / task_button_poll() / task_uart_heartbeat()
```

- Each task is registered once with `scheduler_add_task(func, period_ms, name)`.
- The ISR (`SysTick_Handler`) is kept intentionally tiny — it only
  decrements counters, it never runs task code directly.
- The scheduler is **cooperative**: a task runs to completion once
  dispatched, then gives control back. No preemption, no per-task stack.
- Each task keeps its own `static enum` state (e.g. LED on/off, button
  idle/confirm/held) so it can do multi-step work without ever blocking.

## What it does on the board

- **`task_led_blink`** — blinks the onboard LED (PC13) every 500ms.
- **`task_button_poll`** — reads the onboard KEY button (PA0) every 20ms,
  debounced (the 20ms scheduling period itself provides the debounce).
  Pressing it toggles the LED blink on/off.
- **`task_uart_heartbeat`** — sends status over USART1 (PA9 = TX, PA10 =
  RX) every 1000ms, so you can watch it running on a serial terminal.

## File overview

| File | Purpose |
|---|---|
| `src/main.c` | Application: hardware init + the three tasks |
| `src/scheduler.c` / `scheduler.h` | Hardware-agnostic scheduler core (TCB table, tick handler, dispatcher) |
| `src/stm32f411_regs.h` | Minimal hand-written register map (RCC, GPIO, SysTick, USART1) |
| `startup/startup.s` | Vector table + `Reset_Handler` (copies `.data`, zeroes `.bss`, calls `main`) |
| `startup/stm32f411.ld` | Linker script — memory map (512KB flash / 128KB RAM) |
| `openocd.cfg` | OpenOCD config for flashing/debugging via ST-Link |
| `Makefile` | Build + flash targets |

## Toolchain required

| Tool | Purpose | Install (Debian/Ubuntu) |
|---|---|---|
| `arm-none-eabi-gcc` / `binutils` | Cross-compiler + linker for Cortex-M4 | `sudo apt install gcc-arm-none-eabi` |
| `make` | Runs the build | usually preinstalled |
| `openocd` | Flash/debug over ST-Link | `sudo apt install openocd` |
| A serial terminal (e.g. `screen`, `minicom`, PuTTY) | View UART heartbeat | `sudo apt install screen` |
| ST-Link probe (or one built into a dev board) | Connects PC to the board's SWD pins | — |

## Build & flash

```bash
make            # compiles + links -> build/scheduler.elf/.bin/.hex
openocd -f openocd.cfg -c "program build/scheduler.elf verify reset exit"
```

Then view the heartbeat:
```bash
screen /dev/ttyUSB0 115200      # or whatever port your USB-TTL adapter enumerates as
```

## Board wiring assumed

- Onboard LED: **PC13** (active-low)
- Onboard KEY button: **PA0** (pulls to GND when pressed)
- UART: **USART1**, PA9 = TX, PA10 = RX, 115200 8N1 (needs an external USB-TTL adapter)
- Clock: default 16 MHz HSI, no PLL configured