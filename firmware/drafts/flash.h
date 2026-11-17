#ifndef SST_FLASH_H
#define SST_FLASH_H
#include <stdint.h>
#define FLASH_SIZE 0x80000UL
#define SECTOR_SIZE 0x1000UL
#define BLOCK_SIZE 64
enum {
    OK = 0, ERR_COMMAND = 1, ERR_BANK = 2, ERR_RANGE = 3,
    ERR_LENGTH = 4, ERR_CRC = 5, ERR_TIMEOUT = 6, ERR_ID = 7,
    ERR_NEEDS_ERASE = 8, ERR_VERIFY = 9, ERR_CONFIRM = 10,
    ERR_VERSION = 11, ERR_UART = 12
};
extern uint32_t flash_error_address;
extern uint8_t flash_error_expected, flash_error_actual;
uint8_t flash_bounds(uint8_t bank, uint32_t address, uint32_t length);
uint8_t flash_id(uint8_t bank, uint8_t out[4]);
uint8_t flash_read_block(uint8_t bank, uint32_t address, uint8_t *out, uint8_t length);
uint8_t flash_erase_sector(uint8_t bank, uint32_t address);
uint8_t flash_program(uint8_t bank, uint32_t address, const uint8_t *data, uint8_t length);
#endif
