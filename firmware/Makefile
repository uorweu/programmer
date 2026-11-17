CC = avr-gcc
CFLAGS = -mmcu=atmega16a -DF_CPU=1000000UL -Os -std=gnu11 -Wall -Wextra -Werror
BUILD_DIR = ../build
PROGRAMMER_SOURCES = programmer.c bus/bus.c flash/flash.c uart/uart.c
PROGRAMMER_HEADERS = hardware/registers.h hardware/pins.h bus/bus.h flash/flash.h uart/uart.h

all: sst-programmer.hex

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/flash-id.elf: legacy/main.c Makefile | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

$(BUILD_DIR)/flash-id.hex: $(BUILD_DIR)/flash-id.elf
	avr-objcopy -O ihex -R .eeprom $< $@

$(BUILD_DIR)/sst-programmer.elf: $(PROGRAMMER_SOURCES) $(PROGRAMMER_HEADERS) Makefile | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $(PROGRAMMER_SOURCES)
	avr-size $@

$(BUILD_DIR)/sst-programmer.hex: $(BUILD_DIR)/sst-programmer.elf
	avr-objcopy -O ihex -R .eeprom $< $@

flash-id.elf: $(BUILD_DIR)/flash-id.elf
flash-id.hex: $(BUILD_DIR)/flash-id.hex
sst-programmer.elf: $(BUILD_DIR)/sst-programmer.elf
sst-programmer.hex: $(BUILD_DIR)/sst-programmer.hex

.PHONY: all flash-id.elf flash-id.hex sst-programmer.elf sst-programmer.hex
