#include "bus.h"
#include "../hardware/registers.h"
#include "../hardware/pins.h"

/* Send bit 23 first: U3 receives the high byte and U1 the low byte. */
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

void deselect(void) {
  PORTD |= (1u << UPPER_CE_BIT);
  PORTD |= (1u << LOWER_CE_BIT);
  PORTD |= (1u << FLASH_OE_BIT);
  PORTD |= (1u << FLASH_WE_BIT);
}

void bus_init(void) {
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
}

void command_write(uint8_t ce_bit, uint32_t address, uint8_t value) {
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

uint8_t bus_read(uint8_t ce_bit, uint32_t address) {
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
