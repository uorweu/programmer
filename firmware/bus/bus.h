#ifndef SST_BUS_H
#define SST_BUS_H

#include <stdint.h>

/* ce_bit is a Port D bit position: UPPER_CE_BIT or LOWER_CE_BIT. */
void bus_init(void);
void deselect(void);
void command_write(uint8_t ce_bit, uint32_t address, uint8_t value);
uint8_t bus_read(uint8_t ce_bit, uint32_t address);

#endif
