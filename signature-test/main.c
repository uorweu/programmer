/* ATmega328P standalone, read-only ATmega16A ISP signature test.
 * No target erase, write, fuse or lock commands are implemented.
 * The result is stored in this tester's EEPROM, never the target's.
 */
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

/* Delay calculations deliberately assume the fastest supported tester clock.
 * At slower clocks the test simply runs more slowly. No fuse changes needed.
 */
#if F_CPU != 20000000UL
#error Build with F_CPU=20000000UL for conservative delays
#endif

#define MOSI PB3 /* DIP pin 17 -> ATmega16A pin 6 */
#define MISO PB4 /* DIP pin 18 <- ATmega16A pin 7 */
#define SCK  PB5 /* DIP pin 19 -> ATmega16A pin 8 */
#define LED  PB0 /* DIP pin 14 -> optional resistor + LED -> GND */
#define TARGET_RESET PD2 /* DIP pin 4 -> ATmega16A pin 9 */
#define START PD3 /* DIP pin 5: leave open at boot, then ground to start */
#define RESCUE_CLOCK PB1 /* DIP pin 15 -> 1 kOhm -> target XTAL1 pin 13 */

static uint8_t slow;

static void half_period(void)
{
    if (slow) { _delay_us(100); }
    else { _delay_us(10); }
}

static uint8_t transfer(uint8_t out)
{
    uint8_t in = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        if (out & 0x80) PORTB |= _BV(MOSI);
        else PORTB &= ~_BV(MOSI);
        out <<= 1;
        half_period();
        PORTB |= _BV(SCK);
        half_period();
        in = (uint8_t)((in << 1) | !!(PINB & _BV(MISO)));
        PORTB &= ~_BV(SCK);
    }
    return in;
}

static uint8_t enable(void)
{
    /* SCK is already low. A positive RESET pulse synchronizes entry. */
    PORTD |= _BV(TARGET_RESET);
    _delay_ms(20);
    PORTD &= ~_BV(TARGET_RESET);
    _delay_ms(30);
    transfer(0xAC);
    transfer(0x53);
    uint8_t echo = transfer(0x00);
    transfer(0x00);
    return echo;
}

static uint8_t signature(uint8_t index)
{
    transfer(0x30);
    transfer(0x00);
    transfer(index);
    return transfer(0x00);
}

static uint8_t read_setting(uint8_t command, uint8_t second)
{
    transfer(command);
    transfer(second);
    transfer(0x00);
    return transfer(0x00);
}

static void probe(uint8_t *record)
{
    /* status, enable echo, signature[3], low fuse, high fuse, lock byte */
    record[0] = 0;
    record[1] = 0;
    for (uint8_t i = 2; i < 8; ++i) record[i] = 0xFF;
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        record[1] = enable();
        if (record[1] != 0x53) continue;
        for (uint8_t i = 0; i < 3; ++i) record[2 + i] = signature(i);
        record[0] = (record[2] == 0x1E && record[3] == 0x94 &&
                     record[4] == 0x03) ? 1 : 2;
        for (uint8_t i = 0; i < 3; ++i) {
            if (signature(i) != record[2 + i]) record[0] = 3;
        }
        /* Interpret these settings only for a consistently identified 16A. */
        if (record[0] == 1) {
            record[5] = read_setting(0x50, 0x00);
            record[6] = read_setting(0x58, 0x08);
            record[7] = read_setting(0x58, 0x00);
            if (read_setting(0x50, 0x00) != record[5] ||
                read_setting(0x58, 0x08) != record[6] ||
                read_setting(0x58, 0x00) != record[7]) record[0] = 4;
        }
        break;
    }
}

int main(void)
{
    cli();
    MCUSR = 0;
    wdt_disable();
    PORTB = 0;
    /* Preserve the previous result when reconnecting USBasp for readback.
     * Require START high first, then low after power-up. A wire left
     * grounded at boot cannot trigger the test. While waiting, target
     * signals remain inputs and EEPROM is untouched.
     */
    DDRD &= ~_BV(START);
    PORTD |= _BV(START);
    _delay_ms(10);
    while (!(PIND & _BV(START))) { }
    while (PIND & _BV(START)) { }

    DDRB = _BV(MOSI) | _BV(SCK) | _BV(LED);
    PORTD &= ~_BV(TARGET_RESET);
    DDRD |= _BV(TARGET_RESET);

    /* Header A5 5A 04 03, followed by three eight-byte probe records.
     * Stages: normal ISP; slower ISP; slower ISP with supplied target clock.
     */
    eeprom_update_byte((uint8_t *)0, 0);
    _delay_ms(100);
    uint8_t result[28] = {0xA5, 0x5A, 4, 3};
    uint8_t passed = 0;
    for (uint8_t stage = 0; stage < 3; ++stage) {
        slow = stage != 0;
        if (stage == 2) {
            /* Hold target reset while its clock starts. OC1A toggles at
             * tester CPU frequency / 2, without interrupts or fuse changes.
             */
            PORTD &= ~_BV(TARGET_RESET);
            DDRB |= _BV(RESCUE_CLOCK);
            TCCR1A = _BV(COM1A0);
            TCNT1 = 0;
            OCR1A = 0;
            TCCR1B = _BV(WGM12) | _BV(CS10);
            _delay_ms(100);
        }
        probe(result + 4 + stage * 8);
        if (result[4 + stage * 8] == 1) passed = 1;
    }
    PORTD &= ~_BV(TARGET_RESET);
    TCCR1B = 0;
    TCCR1A = 0;
    DDRB &= ~_BV(RESCUE_CLOCK);
    /* Release all target signals, then commit the completion marker last. */
    DDRB &= ~(_BV(MOSI) | _BV(SCK));
    PORTD |= _BV(TARGET_RESET);
    DDRD &= ~_BV(TARGET_RESET);
    PORTD &= ~_BV(TARGET_RESET);
    eeprom_update_block(result + 1, (void *)1, sizeof result - 1);
    eeprom_update_byte((uint8_t *)0, result[0]);
    eeprom_busy_wait();

    if (passed) PORTB |= _BV(LED);
    for (;;) {
        if (!passed) {
            PORTB ^= _BV(LED);
            _delay_ms(100);
        }
    }
}
