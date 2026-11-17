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
static void deselect(void) {
  PORTD |= (1u << UPPER_CE_BIT);
  PORTD |= (1u << LOWER_CE_BIT);
  PORTD |= (1u << FLASH_OE_BIT);
  PORTD |= (1u << FLASH_WE_BIT);
}
static void command_write(uint8_t ce_bit, uint32_t address, uint8_t value) {
  deselect();
  address_set(address);
  PORTA = value;
  DDRA = 0xFF;
  PORTD &= ~(1u << ce_bit);
  PORTD &= ~(1u << FLASH_WE_BIT);
  PORTD |= (1u << FLASH_WE_BIT);
  deselect();
  DDRA = 0x00;
  PORTA = 0x00;
}
static void flash_id_enter(uint8_t ce_bit) {
  command_write(ce_bit, 0x5555UL, 0xAA);
  command_write(ce_bit, 0x2AAAUL, 0x55);
  command_write(ce_bit, 0x5555UL, 0x90);
}
static void flash_id_exit(uint8_t ce_bit) {
  command_write(ce_bit, 0x0000UL, 0xF0);
}
static uint8_t flash_read(uint8_t ce_bit, uint32_t address) {
  deselect();
  DDRA = 0x00;
  PORTA = 0x00;
  address_set(address);
  PORTD &= ~(1u << ce_bit);
  PORTD &= ~(1u << FLASH_OE_BIT);
  __asm__ __volatile__("nop\n\tnop");
  uint8_t value = PINA;
  deselect();
  return value;
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

static void uart_hex(uint8_t value) {
  const char digits[] = "0123456789ABCDEF";
  uart_put(digits[value >> 4]);
  uart_put(digits[value & 0x0F]);
}
static void flash_show_id(uint8_t ce_bit) {
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
