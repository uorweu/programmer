#include "flash.h"
#include "../bus/bus.h"
#include "../uart/uart.h"

static void flash_unlock(uint8_t ce_bit) {
  command_write(ce_bit, 0x5555UL, 0xAA);
  command_write(ce_bit, 0x2AAAUL, 0x55);
}

void flash_program_start(uint8_t ce_bit, uint32_t address,
                         uint8_t value) {
  flash_unlock(ce_bit);
  command_write(ce_bit, 0x5555UL, 0xA0);
  command_write(ce_bit, address, value);
}

static void flash_id_enter(uint8_t ce_bit) {
  command_write(ce_bit, 0x5555UL, 0xAA);
  command_write(ce_bit, 0x2AAAUL, 0x55);
  command_write(ce_bit, 0x5555UL, 0x90);
}

static void flash_id_exit(uint8_t ce_bit) {
  command_write(ce_bit, 0x0000UL, 0xF0);
}

uint8_t flash_read(uint8_t ce_bit, uint32_t address) {
  return bus_read(ce_bit, address);
}

uint8_t flash_wait_program(uint8_t ce_bit, uint32_t address,
                           uint8_t expected) {
  for (uint16_t attempt = 0; attempt < 1000; attempt++) {
    uint8_t value = flash_read(ce_bit, address);
    if ((value & 0x80) == (expected & 0x80)) {
      return flash_read(ce_bit, address) == expected;
    }
  }
  return 0;
}

void flash_show_id(uint8_t ce_bit) {
  flash_id_enter(ce_bit);
  uint8_t manufacturer = flash_read(ce_bit, 0);
  uint8_t device = flash_read(ce_bit, 1);
  flash_id_exit(ce_bit);
  uart_text("Manufacturer=0x");
  uart_hex(manufacturer);
  uart_text(" Device=0x");
  uart_hex(device);
  uart_text("\r\n");
}
