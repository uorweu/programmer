CC = avr-gcc
CFLAGS = -mmcu=atmega328p -DF_CPU=20000000UL -Os -std=gnu11 -Wall -Wextra -Werror

all: signature-test.hex

signature-test.elf: main.c
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

signature-test.hex: signature-test.elf
	avr-objcopy -O ihex -R .eeprom $< $@

clock-only.elf: clock-only.c
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

clock-only.hex: clock-only.elf
	avr-objcopy -O ihex -R .eeprom $< $@

recover-clock.elf: recover-clock.c
	$(CC) $(CFLAGS) -o $@ $<
	avr-size $@

recover-clock.hex: recover-clock.elf
	avr-objcopy -O ihex -R .eeprom $< $@

.PHONY: all
