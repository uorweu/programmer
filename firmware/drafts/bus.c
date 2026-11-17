/* Pin allocation is identical to the bench-tested ID checker. */
#include "bus.h"
#include <avr/io.h>
#include <util/delay.h>

#define CONTROLS (_BV(PD2) | _BV(PD3) | _BV(PD4) | _BV(PD5))
static uint32_t last_address;
static uint8_t address_valid;

void bus_idle(void)
{
    PORTD |= CONTROLS;
    DDRA = 0;
    PORTA = 0;
}

static void address_set(uint32_t address)
{
    if (address_valid && last_address == address) return;
    PORTB &= ~(_BV(PB1) | _BV(PB2));
    for (int8_t byte = 2; byte >= 0; --byte) {
        uint8_t value = (uint8_t)(address >> (byte * 8));
        for (uint8_t bit = 0; bit < 8; ++bit) {
            if (value & 0x80) PORTB |= _BV(PB0);
            else PORTB &= ~_BV(PB0);
            _delay_us(1);
            PORTB |= _BV(PB1);
            _delay_us(1);
            PORTB &= ~_BV(PB1);
            value <<= 1;
        }
    }
    PORTB |= _BV(PB2);
    _delay_us(1);
    PORTB &= ~_BV(PB2);
    last_address = address;
    address_valid = 1;
}

void bus_init(void)
{
    PORTD |= CONTROLS; /* Latches HIGH before enabling outputs. */
    DDRD |= CONTROLS;
    bus_idle();
    PORTB &= ~(_BV(PB0) | _BV(PB1) | _BV(PB2));
    DDRB |= _BV(PB0) | _BV(PB1) | _BV(PB2);
    address_valid = 0;
    address_set(0);
}

void bus_write(uint8_t bank, uint32_t address, uint8_t value)
{
    bus_idle();
    address_set(address);
    PORTA = value;
    DDRA = 0xFF;
    _delay_us(1);
    PORTD &= ~(bank ? _BV(PD3) : _BV(PD2));
    _delay_us(1);
    PORTD &= ~_BV(PD5);
    _delay_us(1);
    PORTD |= _BV(PD5);
    _delay_us(1);
    bus_idle();
}

uint8_t bus_read(uint8_t bank, uint32_t address)
{
    bus_idle();
    address_set(address);
    PORTD &= ~(bank ? _BV(PD3) : _BV(PD2));
    PORTD &= ~_BV(PD4);
    _delay_us(1);
    uint8_t value = PINA;
    bus_idle();
    return value;
}

void bus_wait_100us(void) { _delay_us(100); }
