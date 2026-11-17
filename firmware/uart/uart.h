#ifndef SST_UART_H
#define SST_UART_H

#include <stdint.h>

void uart_init(void);
uint8_t uart_available(void);
/* Read only when uart_available() is nonzero. */
uint8_t uart_get(void);
void uart_put(uint8_t c);
void uart_text(const char *s);
void uart_hex(uint8_t value);

#endif
