#ifndef SST_FLASH_H
#define SST_FLASH_H

#include <stdint.h>

uint8_t flash_read(uint8_t ce_bit, uint32_t address);
void flash_show_id(uint8_t ce_bit);

/* Learning-stage programming functions: not yet tested on hardware.
 * No terminal command calls these functions. Program start is asynchronous;
 * the caller must validate the address/data and then wait for completion.
 */
void flash_program_start(uint8_t ce_bit, uint32_t address, uint8_t value);
uint8_t flash_wait_program(uint8_t ce_bit, uint32_t address, uint8_t expected);

#endif
