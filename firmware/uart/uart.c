#include "uart.h"
#include "../hardware/registers.h"

/* 9600 baud, 8N1, with the recovered 1 MHz MCU clock. */
void uart_init(void) {
  UBRRH = 0;
  UBRRL = 12;
  UCSRA = (1u << 1);
  UCSRB = (1u << TXEN_BIT) | (1u << RXEN_BIT);
  UCSRC = (1u << URSEL_BIT) | (1u << UCSZ1_BIT) | (1u << UCSZ0_BIT);
}

uint8_t uart_available(void) {
  return (UCSRA & (1u << RXC_BIT)) != 0;
}

/* Call only after uart_available() reports a received byte. */
uint8_t uart_get(void) {
  return UDR;
}

void uart_put(uint8_t c) {
  while (!(UCSRA & (1u << UDRE_BIT))) {
  }
  UDR = c;
}

void uart_text(const char *s) {
  while (*s)
    uart_put(*s++);
}

void uart_hex(uint8_t value) {
  const char digits[] = "0123456789ABCDEF";
  uart_put(digits[value >> 4]);
  uart_put(digits[value & 0x0F]);
}
