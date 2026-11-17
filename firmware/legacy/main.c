/* ATmega16A, internal 1 MHz clock: SST39SF040 identification only.
 * Commands enter/exit software ID mode; no erase or program sequences exist.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>

#define BAUD 9600
#include <util/setbaud.h>

#if F_CPU != 1000000UL
#error This initial firmware expects the recovered internal 1 MHz clock
#endif

#define SERIAL_DATA PB0
#define SHIFT_CLOCK PB1
#define LATCH_CLOCK PB2
#define UPPER_CE PD2
#define LOWER_CE PD3
#define FLASH_OE PD4
#define FLASH_WE PD5
#define CE_MASK (_BV(UPPER_CE) | _BV(LOWER_CE))
#define CONTROL_MASK (CE_MASK | _BV(FLASH_OE) | _BV(FLASH_WE))

static void deselect(void)
{
    PORTD |= CONTROL_MASK;
}

static void address_set(uint32_t address)
{
    /* Every caller first disables both Flash chips. Send bit 23 first:
     * U3 holds bits 16..23, U2 bits 8..15, U1 bits 0..7.
     */
    PORTB &= ~(_BV(SHIFT_CLOCK) | _BV(LATCH_CLOCK));
    for (uint32_t mask = 0x800000UL; mask; mask >>= 1) {
        if (address & mask) PORTB |= _BV(SERIAL_DATA);
        else PORTB &= ~_BV(SERIAL_DATA);
        _delay_us(1);
        PORTB |= _BV(SHIFT_CLOCK);
        _delay_us(1);
        PORTB &= ~_BV(SHIFT_CLOCK);
    }
    PORTB |= _BV(LATCH_CLOCK);
    _delay_us(1);
    PORTB &= ~_BV(LATCH_CLOCK);
}

static void command_write(uint8_t ce_bit, uint32_t address, uint8_t value)
{
    deselect();
    address_set(address);
    PORTA = value;
    DDRA = 0xFF;
    _delay_us(1);
    PORTD &= ~_BV(ce_bit);
    _delay_us(1);
    PORTD &= ~_BV(FLASH_WE);
    _delay_us(1);
    PORTD |= _BV(FLASH_WE);
    _delay_us(1);
    deselect();
    DDRA = 0;
    PORTA = 0; /* Disable data input pull-ups. */
}

static uint8_t flash_read(uint8_t ce_bit, uint32_t address)
{
    deselect();
    DDRA = 0;
    PORTA = 0;
    address_set(address);
    PORTD &= ~_BV(ce_bit);
    PORTD &= ~_BV(FLASH_OE);
    _delay_us(1);
    uint8_t value = PINA;
    deselect();
    return value;
}

static void uart_put(char c)
{
    while (!(UCSRA & _BV(UDRE))) { }
    UDR = c;
}

static void uart_text(const char *s)
{
    while (*s) uart_put(*s++);
}

static void uart_hex(uint8_t value)
{
    static const char digits[] = "0123456789ABCDEF";
    uart_put(digits[value >> 4]);
    uart_put(digits[value & 0x0F]);
}

static void uart_flush(void)
{
    while (!(UCSRA & _BV(TXC))) { }
}

static void identify(uint8_t ce_bit, const char *name)
{
    uart_text("ID START ");
    uart_text(name);
    uart_text("\r\n");
    uart_flush(); /* Preserve the marker before any bus-related fault. */
    /* Reset any residual ID state, then issue only software-ID commands. */
    command_write(ce_bit, 0, 0xF0);
    _delay_us(1);
    command_write(ce_bit, 0x5555, 0xAA);
    command_write(ce_bit, 0x2AAA, 0x55);
    command_write(ce_bit, 0x5555, 0x90);
    _delay_us(1);
    uint8_t manufacturer = flash_read(ce_bit, 0);
    uint8_t device = flash_read(ce_bit, 1);
    uint8_t manufacturer_again = flash_read(ce_bit, 0);
    uint8_t device_again = flash_read(ce_bit, 1);
    command_write(ce_bit, 0, 0xF0);
    _delay_us(1);

    uart_text(name);
    uart_text(": manufacturer=");
    uart_hex(manufacturer);
    uart_text(" device=");
    uart_hex(device);
    if (manufacturer != manufacturer_again || device != device_again)
        uart_text(" UNSTABLE\r\n");
    else if (manufacturer == 0xBF && device == 0xB7)
        uart_text(" SST39SF040 OK\r\n");
    else
        uart_text(" UNEXPECTED (expected BF B7)\r\n");
}

static void hold_read(uint8_t ce_bit, const char *name)
{
    command_write(ce_bit, 0, 0xF0);
    _delay_us(1);
    deselect();
    DDRA = 0;
    PORTA = 0;
    address_set(0);
    PORTD &= ~_BV(ce_bit);
    PORTD &= ~_BV(FLASH_OE);
    _delay_us(1);
    uint8_t data = PINA;
    uart_text(name);
    uart_text(" held READ at address 0: data=");
    uart_hex(data);
    uart_text(". Selected CE=0V, OE=0V, WE=5V; other CE=5V.\r\n");
}

static void hold_address(uint32_t address)
{
    deselect();
    DDRA = 0;
    PORTA = 0;
    address_set(address);
    uart_text("Address held; both chips disabled.\r\n");
}

static void help(void)
{
    uart_text("\r\nATmega16A Flash ID checker - 9600 8N1\r\n");
    uart_text("i: IDs; u/l: hold UPPER/LOWER read; x: idle; h: help\r\n");
    uart_text("0/1: address all LOW/HIGH; a/b: alternating address bits\r\n");
    uart_text("No erase/program commands.\r\n");
}

int main(void)
{
    cli();
    uint8_t reset_cause = MCUCSR;
    MCUCSR = 0;
    wdt_disable();

    /* Inactive chip controls before enabling outputs; external 10k pull-ups
     * keep these inactive during reset and ISP programming as well.
     */
    deselect();
    DDRD |= CONTROL_MASK;
    DDRA = 0;
    PORTA = 0;
    PORTB &= ~(_BV(SERIAL_DATA) | _BV(SHIFT_CLOCK) | _BV(LATCH_CLOCK));
    DDRB |= _BV(SERIAL_DATA) | _BV(SHIFT_CLOCK) | _BV(LATCH_CLOCK);
    address_set(0);

    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
#if USE_2X
    UCSRA = _BV(U2X);
#else
    UCSRA = 0;
#endif
    UCSRB = _BV(RXEN) | _BV(TXEN);
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
    _delay_ms(100);
    uart_text("\r\nBOOT v2 reset=0x");
    uart_hex(reset_cause);
    uart_text("\r\n");
    help();

    for (;;) {
        if (!(UCSRA & _BV(RXC))) continue;
        uint8_t status = UCSRA;
        char c = UDR;
        if (status & (_BV(FE) | _BV(DOR) | _BV(UPE))) {
            uart_text("UART error; resend command.\r\n");
            continue;
        }
        if (c != '\r' && c != '\n' && c != ' ') {
            uart_text("RX=0x");
            uart_hex((uint8_t)c);
            uart_text("\r\n");
            uart_flush();
        }
        if (c == 'i' || c == 'I') {
            identify(UPPER_CE, "UPPER");
            identify(LOWER_CE, "LOWER");
        } else if (c == 'u' || c == 'U') {
            hold_read(UPPER_CE, "UPPER");
        } else if (c == 'l' || c == 'L') {
            hold_read(LOWER_CE, "LOWER");
        } else if (c == 'x' || c == 'X') {
            deselect();
            DDRA = 0;
            PORTA = 0;
            uart_text("Idle: both CEs, OE and WE HIGH; data pins inputs.\r\n");
        } else if (c == '0' || c == '1') {
            hold_address(c == '0' ? 0 : 0x7FFFFUL);
        } else if (c == 'a' || c == 'A') {
            hold_address(0x55555UL);
        } else if (c == 'b' || c == 'B') {
            hold_address(0x2AAAAUL);
        } else if (c == 'h' || c == 'H' || c == '?') {
            help();
        } else if (c != '\r' && c != '\n' && c != ' ') {
            uart_text("Unknown command; use h for help.\r\n");
        }
    }
}
