##############################################################################
# RTOS Industrial Data Logger - STM32F411CEU6 - arm-none-eabi-gcc + make
##############################################################################

TARGET     = rtos_logger
BUILD_DIR  = build

PREFIX     = arm-none-eabi-
CC         = $(PREFIX)gcc
AS         = $(PREFIX)gcc -x assembler-with-cpp
OBJCOPY    = $(PREFIX)objcopy
SIZE       = $(PREFIX)size
GDB        = $(PREFIX)gdb

MCU_FLAGS  = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

# ---- Sources ---------------------------------------------------------------
C_SOURCES = \
  Core/Src/main.c \
  Core/Src/stm32f4xx_it.c \
  Core/Src/bsp_uart.c \
  Core/Src/bsp_i2c.c \
  Core/Src/bsp_spi_sd.c \
  Core/Src/ssd1306.c \
  Core/Src/sensor_task.c \
  Core/Src/logger_task.c \
  Core/Src/display_task.c \
  Core/Src/cli_task.c \
  Core/Src/monitor_task.c \
  Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c \
  Middlewares/FreeRTOS/tasks.c \
  Middlewares/FreeRTOS/queue.c \
  Middlewares/FreeRTOS/list.c \
  Middlewares/FreeRTOS/timers.c \
  Middlewares/FreeRTOS/event_groups.c \
  Middlewares/FreeRTOS/stream_buffer.c \
  Middlewares/FreeRTOS/portable/GCC/ARM_CM4F/port.c \
  Middlewares/FreeRTOS/portable/MemMang/heap_4.c

ASM_SOURCES = \
  Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f411xe.s

# ---- Includes ---------------------------------------------------------------
C_INCLUDES = \
  -ICore/Inc \
  -IDrivers/CMSIS/Include \
  -IDrivers/CMSIS/Device/ST/STM32F4xx/Include \
  -IMiddlewares/FreeRTOS/include \
  -IMiddlewares/FreeRTOS/portable/GCC/ARM_CM4F

C_DEFS = -DSTM32F411xE

# ---- Flags -------------------------------------------------------------------
CFLAGS  = $(MCU_FLAGS) $(C_DEFS) $(C_INCLUDES) -Wall -Wextra -O2 -g3 \
          -ffunction-sections -fdata-sections -fno-common -fstack-usage \
          -std=gnu11 -MMD -MP

ASFLAGS = $(MCU_FLAGS) -g3

LDSCRIPT = STM32F411CEU6_FLASH.ld
LDFLAGS  = $(MCU_FLAGS) -specs=nano.specs -specs=nosys.specs \
           -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref \
           -Wl,--gc-sections -static \
           -Wl,--start-group -lc -lm -Wl,--end-group

# ---- Object lists -------------------------------------------------------------
OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# ---- Rules ---------------------------------------------------------------------
all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin
	$(SIZE) $(BUILD_DIR)/$(TARGET).elf

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(OBJCOPY) -O binary -S $< $@

$(BUILD_DIR):
	mkdir -p $@

# ---- Flash / debug helpers (adjust to whatever probe you actually have) ------
flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x8000000

flash-openocd: $(BUILD_DIR)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
	  -c "program $< verify reset exit"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean flash flash-openocd

-include $(wildcard $(BUILD_DIR)/*.d)
