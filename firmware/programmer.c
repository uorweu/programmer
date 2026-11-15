#include "hardware/pins.h"
#include "hardware/registers.h"
// 24 bits address for 3 595s - 10000000 0000000 0000000 = 0x800000UL
static void address_set(uint32_t address) {
  for (uint32_t mask = 0x800000UL; mask != 0; mask >>= 1) {
    /* shift bits in mask 1 position to the right and then save to mask*/
    if (address & mask) {
      PORTB |= (1u << ADDRESS_DATA_BIT);
    } else {
      PORTB &= ~(1u << ADDRESS_DATA_BIT);
    }
    PORTB |= (1u << ADDRESS_SHIFT_BIT);
    PORTB &= ~(1u << ADDRESS_SHIFT_BIT);
  }
  PORTB |= (1u << ADDRESS_LATCH_BIT);
  PORTB &= ~(1u << ADDRESS_LATCH_BIT);
}

static void uart_put(uint8_t c) {
  while (!(UCSRA & (1u << UDRE_BIT))) {
  }
  UDR = c;
}

static void uart_text(const char *s) {
  while (*s)
    uart_put(*s++);
}

int main(void) {
  PORTD |= (1u << UPPER_CE_BIT); // set inactive
  PORTD |= (1u << LOWER_CE_BIT);
  PORTD |= (1u << FLASH_OE_BIT);
  PORTD |= (1u << FLASH_WE_BIT);

  // set the pins as output
  DDRD |= (1u << UPPER_CE_BIT); // in DDRD 1 = output / 0 = input
  DDRD |= (1u << LOWER_CE_BIT);
  DDRD |= (1u << FLASH_OE_BIT);
  DDRD |= (1u << FLASH_WE_BIT);

  DDRA = 0x00;
  PORTA = 0x00;

  PORTB &= ~(1u << ADDRESS_DATA_BIT);
  PORTB &= ~(1u << ADDRESS_SHIFT_BIT);
  PORTB &= ~(1u << ADDRESS_LATCH_BIT);

  DDRB |= (1u << ADDRESS_DATA_BIT);
  DDRB |= (1u << ADDRESS_SHIFT_BIT);
  DDRB |= (1u << ADDRESS_LATCH_BIT);

  // UART setup
  UBRRH = 0;
  UBRRL = 12;
  UCSRA = (1u << 1);
  UCSRB = (1u << TXEN_BIT) | (1u << RXEN_BIT);
  UCSRC = (1u << URSEL_BIT) | (1u << UCSZ1_BIT) | (1u << UCSZ0_BIT);
  for (;;) {
    // echo on picocom
    if (!(UCSRA & (1u << RXC_BIT)))
      continue;
    uint8_t c = UDR;
    uart_text("RX=");
    uart_put(c);
    uart_text("\r\n");
  }
}
