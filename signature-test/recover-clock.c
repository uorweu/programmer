/* ATmega328P standalone ATmega16A recovery.
 * DESTRUCTIVE: erases target Flash to clear lock bits, then restores
 * ATmega16A default low/high fuses E1/99. EESAVE is left as found at erase.
 * Starts only after DIP pin 5 is seen HIGH and subsequently grounded.
 */
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

#if F_CPU != 20000000UL
#error Use 20000000UL as the conservative delay bound
#endif

#define MOSI PB3
#define MISO PB4
#define SCK PB5
#define CLOCK PB1
#define LED PB0
#define RESET PD2
#define START PD3

static uint8_t transfer(uint8_t value)
{
    uint8_t in = 0;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        if (value & 0x80) PORTB |= _BV(MOSI);
        else PORTB &= ~_BV(MOSI);
        value <<= 1;
        _delay_us(100);
        PORTB |= _BV(SCK);
        _delay_us(100);
        in = (uint8_t)((in << 1) | !!(PINB & _BV(MISO)));
        PORTB &= ~_BV(SCK);
    }
    return in;
}

static uint8_t command(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    transfer(a);
    transfer(b);
    transfer(c);
    return transfer(d);
}

static uint8_t enable(void)
{
    for (uint8_t attempt = 0; attempt < 3; ++attempt) {
        PORTD |= _BV(RESET);
        _delay_ms(100);
        PORTD &= ~_BV(RESET);
        _delay_ms(30);
        transfer(0xAC);
        transfer(0x53);
        uint8_t echo = transfer(0);
        transfer(0);
        if (echo == 0x53) return 1;
    }
    return 0;
}

static uint8_t identify(uint8_t *sig)
{
    for (uint8_t i = 0; i < 3; ++i) sig[i] = command(0x30, 0, i, 0);
    if (sig[0] != 0x1E || sig[1] != 0x94 || sig[2] != 0x03) return 0;
    for (uint8_t i = 0; i < 3; ++i) {
        if (command(0x30, 0, i, 0) != sig[i]) return 0;
    }
    return 1;
}

static uint8_t settings(uint8_t *out)
{
    out[0] = command(0x50, 0, 0, 0);
    out[1] = command(0x58, 8, 0, 0);
    out[2] = command(0x58, 0, 0, 0);
    return command(0x50, 0, 0, 0) == out[0] &&
           command(0x58, 8, 0, 0) == out[1] &&
           command(0x58, 0, 0, 0) == out[2];
}

int main(void)
{
    cli();
    MCUSR = 0;
    wdt_disable();
    DDRB = 0;
    PORTB = 0;
    DDRD &= ~(_BV(START) | _BV(RESET));
    PORTD = _BV(START);
    _delay_ms(10);
    while (!(PIND & _BV(START))) { }
    while (PIND & _BV(START)) { }

    /* EEPROM: A5 5A 05, status, sig[3], before settings[3],
     * after settings[3], progress flags, reserved[2].
     * Status: 1=verified success, 2=bad signature, 3=no enable echo,
     * 4=erase/unlock failed, 5=fuse verification failed,
     * 6=no signature after supplied clock stopped,
     * 7=unstable initial fuse/lock reads.
     */
    uint8_t result[16] = {0xA5, 0x5A, 5, 3,
                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                         0xFF, 0xFF, 0xFF, 0, 0, 0};
    eeprom_update_byte((uint8_t *)0, 0);
    DDRB = _BV(MOSI) | _BV(SCK) | _BV(CLOCK) | _BV(LED);
    PORTD &= ~_BV(RESET);
    DDRD |= _BV(RESET);
    TCCR1A = _BV(COM1A0);
    TCNT1 = 0;
    OCR1A = 0;
    TCCR1B = _BV(WGM12) | _BV(CS10);
    _delay_ms(100);

    if (!enable()) goto finish;
    result[3] = 2;
    if (!identify(result + 4)) goto finish;
    result[3] = 7;
    if (!settings(result + 7)) goto finish;

    /* Clear protection only if the chip is locked. Chip erase retains
     * current fuse settings. With the reported high fuse 00, EESAVE=0
     * preserves EEPROM; target Flash contents are erased.
     */
    if ((result[9] & 3) != 3) {
        command(0xAC, 0x80, 0, 0);
        _delay_ms(50);
        result[13] |= 1;
        result[3] = 4;
        if (!enable()) goto finish;
        if (!identify(result + 4)) goto finish;
        if ((command(0x58, 0, 0, 0) & 3) != 3) goto finish;
    }

    /* Default high fuse: ISP enabled, reset vector in application section,
     * JTAG enabled as at the factory. Low fuse: internal 1 MHz RC clock.
     */
    command(0xAC, 0xA8, 0, 0x99);
    _delay_ms(20);
    result[13] |= 2;
    command(0xAC, 0xA0, 0, 0xE1);
    _delay_ms(20);
    result[13] |= 4;
    result[3] = 5;
    if (!settings(result + 10)) goto finish;
    if (result[10] != 0xE1 || result[11] != 0x99 ||
        (result[12] & 3) != 3) goto finish;

    /* Exit programming to latch new fuses, then remove supplied clock and
     * re-enter ISP using the target's internal oscillator alone.
     */
    PORTD |= _BV(RESET);
    _delay_ms(100);
    PORTD &= ~_BV(RESET);
    TCCR1B = 0;
    TCCR1A = 0;
    DDRB &= ~_BV(CLOCK);
    result[3] = 6;
    if (!enable()) goto finish;
    if (!identify(result + 4)) goto finish;
    if (!settings(result + 10)) goto finish;
    if (result[10] != 0xE1 || result[11] != 0x99 ||
        (result[12] & 3) != 3) goto finish;
    result[13] |= 8;
    result[3] = 1;

finish:
    /* On a failure, keep the rescue clock running so the target retains
     * a clock until power is removed. On success it is already stopped.
     */
    DDRB &= ~(_BV(MOSI) | _BV(SCK));
    PORTD |= _BV(RESET);
    DDRD &= ~_BV(RESET);
    PORTD &= ~_BV(RESET);
    eeprom_update_block(result + 1, (void *)1, sizeof result - 1);
    eeprom_update_byte((uint8_t *)0, result[0]);
    eeprom_busy_wait();
    if (result[3] == 1) PORTB |= _BV(LED);
    for (;;) {
        if (result[3] != 1) {
            PORTB ^= _BV(LED);
            _delay_ms(100);
        }
    }
}
