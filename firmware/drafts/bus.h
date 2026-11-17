#ifndef SST_BUS_H
#define SST_BUS_H
#include <stdint.h>
void bus_init(void);
void bus_idle(void);
void bus_write(uint8_t bank, uint32_t address, uint8_t value);
uint8_t bus_read(uint8_t bank, uint32_t address);
void bus_wait_100us(void);
#endif
