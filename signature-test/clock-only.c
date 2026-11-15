/* Standalone clock source: ATmega328P DIP pin 15 (PB1/OC1A).
 * No target programming commands, EEPROM writes, or fuse changes.
 * Output frequency is the current tester CPU clock divided by two.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

int main(void)
{
    cli();
    MCUSR = 0;
    wdt_disable();
    PORTB &= ~_BV(PB1);
    DDRB |= _BV(PB1);
    TCCR1A = _BV(COM1A0);
    TCNT1 = 0;
    OCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);
    for (;;) { }
}
