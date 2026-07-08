

TARGET     = scheduler
BUILD_DIR  = build

CC_ARM     = arm-none-eabi-gcc
OBJCOPY    = arm-none-eabi-objcopy
SIZE       = arm-none-eabi-size

ARM_ASM    = startup/startup.s
LDSCRIPT   = startup/stm32f411.ld

ARM_CFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
             -Iinc -Wall -O2 -ffunction-sections -fdata-sections \
             -fno-common -std=gnu11

ARM_LDFLAGS = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard \
              -T$(LDSCRIPT) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
              --specs=nano.specs --specs=nosys.specs

ARM_OBJS = $(BUILD_DIR)/startup.o \
           $(BUILD_DIR)/scheduler.o \
           $(BUILD_DIR)/main.o

$(BUILD_DIR)/startup.o: $(ARM_ASM)
	mkdir -p $(BUILD_DIR)
	$(CC_ARM) $(ARM_CFLAGS) -c $< -o $@

$(BUILD_DIR)/scheduler.o: src/scheduler.c
	mkdir -p $(BUILD_DIR)
	$(CC_ARM) $(ARM_CFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: src/main.c
	mkdir -p $(BUILD_DIR)
	$(CC_ARM) $(ARM_CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(ARM_OBJS)
	$(CC_ARM) $(ARM_LDFLAGS) $(ARM_OBJS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

flash: $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex
	@echo ""
	@echo "Built $(BUILD_DIR)/$(TARGET).bin and .hex -- ready to flash."
	@echo "Put the board in DFU mode (hold BOOT0, tap RESET, release BOOT0), then:"
	@echo "  dfu-util -a 0 -s 0x08000000:leave -D $(BUILD_DIR)/$(TARGET).bin"
	@echo "Or, if using an ST-Link probe instead of DFU:"
	@echo "  st-flash write $(BUILD_DIR)/$(TARGET).bin 0x8000000"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: flash clean