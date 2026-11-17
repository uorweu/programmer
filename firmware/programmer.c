#include "hardware/pins.h"
#include "bus/bus.h"
#include "flash/flash.h"
#include "uart/uart.h"

int main(void) {
  bus_init();
  uart_init();

  for (;;) {
    // echo on picocom
    if (!uart_available())
      continue;
    uint8_t c = uart_get();
    if (c == 'u') {
      uint8_t value = flash_read(UPPER_CE_BIT, 0);
      uart_text("UPPER[0] = 0x");
      uart_hex(value);
      uart_text("\r\n");
      continue;
    }
    uart_text("RX=");
    uart_put(c);
    uart_text("\r\n");
    if (c == 'l') {
      uint8_t value = flash_read(LOWER_CE_BIT, 0);
      uart_text("LOWER[0] = 0x");
      uart_hex(value);
      uart_text("\r\n");
      continue;
    }
    if (c == 'i') {
      uart_text("UPPER: ");
      flash_show_id(UPPER_CE_BIT);
      uart_text("LOWER: ");
      flash_show_id(LOWER_CE_BIT);
      continue;
    }
  }
}
