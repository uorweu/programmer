/* Production transport for the bench-tested 1 MHz ATmega16A wiring. */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "bus.h"
#include "flash.h"
#include "protocol.h"
#include "crc16.h"
#define BAUD 9600
#include <util/setbaud.h>
#if F_CPU != 1000000UL
#error This build requires the recovered internal 1 MHz clock
#endif

static uint8_t header[HEADER_SIZE], payload[BLOCK_SIZE], response[BLOCK_SIZE + 1];

static void put(uint8_t value)
{
    while (!(UCSRA & _BV(UDRE))) { }
    UDR = value;
}

static void text_p(PGM_P p)
{
    char c;
    while ((c = pgm_read_byte(p++))) put((uint8_t)c);
}

static void hex(uint8_t value)
{
    uint8_t a = value >> 4, b = value & 15;
    put(a < 10 ? '0' + a : 'A' + a - 10);
    put(b < 10 ? '0' + b : 'A' + b - 10);
}

/* In-frame inter-byte timeout, about 250 ms at the specified MCU clock.
 * All payload bytes and CRC are received before protocol_handle can mutate
 * Flash. The host sends one frame then waits, so no RX IRQ/buffer is needed.
 */
static int16_t get_timed(void)
{
    for (uint16_t count = 0; count < 2500; ++count) {
        if (UCSRA & _BV(RXC)) {
            uint8_t status = UCSRA, value = UDR;
            if (status & (_BV(FE) | _BV(DOR) | _BV(UPE))) return -2;
            return value;
        }
        _delay_us(100);
    }
    return -1;
}

static void send_response(uint8_t length)
{
    put(0xA5); put(0x5A);
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < HEADER_SIZE; ++i) {
        uint8_t value = header[i];
        if (i == 0) value = PROTOCOL_VERSION;
        if (i == 1) value |= 0x80;
        if (i == 7) value = length;
        put(value);
        crc = crc16_byte(crc, value);
    }
    for (uint8_t i = 0; i < length; ++i) {
        put(response[i]);
        crc = crc16_byte(crc, response[i]);
    }
    put((uint8_t)crc); put((uint8_t)(crc >> 8));
}

static void receive_frame(void)
{
    uint16_t crc = 0xFFFF;
    int16_t value;
    uint8_t error = OK;
    for (uint8_t i = 0; i < HEADER_SIZE; ++i) header[i] = 0;
    for (uint8_t i = 0; i < HEADER_SIZE; ++i) {
        value = get_timed();
        if (value < 0) { error = value == -1 ? ERR_TIMEOUT : ERR_UART; goto fail; }
        header[i] = (uint8_t)value;
        crc = crc16_byte(crc, (uint8_t)value);
    }
    if (header[7] > BLOCK_SIZE) {
        /* Drain a bounded oversized frame without storing its payload. */
        for (uint16_t i = 0; i < (uint16_t)header[7] + 2; ++i)
            if (get_timed() < 0) break;
        error = ERR_LENGTH;
        goto fail;
    }
    for (uint8_t i = 0; i < header[7]; ++i) {
        value = get_timed();
        if (value < 0) { error = value == -1 ? ERR_TIMEOUT : ERR_UART; goto fail; }
        payload[i] = (uint8_t)value;
        crc = crc16_byte(crc, (uint8_t)value);
    }
    value = get_timed();
    if (value < 0) { error = value == -1 ? ERR_TIMEOUT : ERR_UART; goto fail; }
    uint16_t received = (uint8_t)value;
    value = get_timed();
    if (value < 0) { error = value == -1 ? ERR_TIMEOUT : ERR_UART; goto fail; }
    received |= (uint16_t)value << 8;
    if (received != crc) { error = ERR_CRC; goto fail; }
    send_response(protocol_handle(header, payload, response));
    bus_idle();
    return;
fail:
    bus_idle();
    response[0] = error;
    send_response(1);
}

static void text_ids(void)
{
    for (uint8_t bank = 0; bank < 2; ++bank) {
        uint8_t id[4];
        uint8_t status = flash_id(bank, id);
        text_p(bank ? PSTR("LOWER: manufacturer=") : PSTR("UPPER: manufacturer="));
        hex(id[0]); text_p(PSTR(" device=")); hex(id[1]);
        text_p(status ? PSTR(" UNEXPECTED (expected BF B7)\r\n") :
                        PSTR(" SST39SF040 OK\r\n"));
    }
}

int main(void)
{
    cli();
    uint8_t reset = MCUCSR;
    MCUCSR = 0;
    wdt_disable();
    bus_init();
    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
    UCSRA = USE_2X ? _BV(U2X) : 0;
    UCSRB = _BV(RXEN) | _BV(TXEN);
    UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
    _delay_ms(100);
    text_p(PSTR("\r\nBOOT programmer v1 reset=0x")); hex(reset);
    text_p(PSTR("\r\ni: IDs; h: help; ROM transfers use tools/sst_programmer.py\r\n"));
    uint8_t sync = 0;
    for (;;) {
        if (!(UCSRA & _BV(RXC))) continue;
        uint8_t status = UCSRA, value = UDR;
        if (status & (_BV(FE) | _BV(DOR) | _BV(UPE))) { sync = 0; continue; }
        if (sync) {
            if (value == 0x5A) { sync = 0; receive_frame(); }
            else sync = value == 0xA5;
        } else if (value == 0xA5) sync = 1;
        else if (value == 'i' || value == 'I') text_ids();
        else if (value == 'h' || value == 'H' || value == '?')
            text_p(PSTR("SST programmer v1, 9600 8N1. i: IDs. Use PC tool for read/erase/write.\r\n"));
    }
}
