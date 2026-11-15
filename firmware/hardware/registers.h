#ifndef REGISTERS_HP
#define REGISTERS_HP
#include <stdint.h>

// Port A
#define PINA                                                                   \
  (*(volatile uint8_t *)0x39) /*read the voltage level on the data pins */
#define DDRA (*(volatile uint8_t *)0x3A)  /*set in/out for Port A8*/
#define PORTA (*(volatile uint8_t *)0x3B) /*A0 - A7 also the Data bus*/

// Port D
#define DDRD (*(volatile uint8_t *)0x31)
#define PORTD (*(volatile uint8_t *)0x32)
/*
 * PD2 - UPPER chip
 * PD3 - LOWER chip
 * PD4 - Enable flash output - /OE
 * PD5 - Enable flash writing - /WE
 */

// PORT B
#define DDRB (*(volatile uint8_t *)0x37)
#define PORTB (*(volatile uint8_t *)0x38)
/*
 * PB0 - send address bits to 595
 * PB1 - Pulse the shift clock to shift each bit in
 * PB2 - Pulse the latch clock to update the address output
 */

// UART
#define UDR (*(volatile uint8_t *)0x2C)   /*buffer*/
#define UCSRA (*(volatile uint8_t *)0x2B) /*status flag*/
#define UCSRB (*(volatile uint8_t *)0x2A) /*enable control */
#define UBRRL (*(volatile uint8_t *)0x29) /*baud rate settings*/
#define UCSRC (*(volatile uint8_t *)0x40) /*frame formating*/
#define UBRRH (*(volatile uint8_t *)0x40) /* still baudrat settings*/

// UCSRA bits
#define UDRE_BIT 5
#define RXC_BIT 7
// UCSRB bits
#define TXEN_BIT 3
#define RXEN_BIT 4

// UCSRC bits
#define URSEL_BIT 7
#define UCSZ1_BIT 2
#define UCSZ0_BIT 1

#endif
