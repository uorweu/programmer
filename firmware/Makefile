CC = avr-gcc
CFLAGS = -mmcu=atmega16a -DF_CPU=1000000UL -Os -std=gnu11 -Wall -Wextra -Werror
BUILD_DIR = ../build

all: sst-programmer.hex

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/flash-id.elf: main.c Makefile | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

$(BUILD_DIR)/flash-id.hex: $(BUILD_DIR)/flash-id.elf
	avr-objcopy -O ihex -R .eeprom $< $@

$(BUILD_DIR)/sst-programmer.elf: programmer.c hardware/registers.h hardware/pins.h Makefile | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ programmer.c
	avr-size $@

$(BUILD_DIR)/sst-programmer.hex: $(BUILD_DIR)/sst-programmer.elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash-id.elf: $(BUILD_DIR)/flash-id.elf
flash-id.hex: $(BUILD_DIR)/flash-id.hex
sst-programmer.elf: $(BUILD_DIR)/sst-programmer.elf
sst-programmer.hex: $(BUILD_DIR)/sst-programmer.hex

.PHONY: all flash-id.elf flash-id.hex sst-programmer.elf sst-programmer.hex
