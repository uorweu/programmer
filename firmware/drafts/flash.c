#include "flash.h"
#include "bus.h"

uint32_t flash_error_address;
uint8_t flash_error_expected, flash_error_actual;

uint8_t flash_bounds(uint8_t bank, uint32_t address, uint32_t length)
{
    if (bank > 1) return ERR_BANK;
    if (!length || address >= FLASH_SIZE || length > FLASH_SIZE - address)
        return ERR_RANGE;
    return OK;
}

static void unlock(uint8_t bank)
{
    bus_write(bank, 0x5555, 0xAA);
    bus_write(bank, 0x2AAA, 0x55);
}

uint8_t flash_id(uint8_t bank, uint8_t out[4])
{
    if (bank > 1) return ERR_BANK;
    bus_write(bank, 0, 0xF0);
    unlock(bank);
    bus_write(bank, 0x5555, 0x90);
    out[0] = bus_read(bank, 0);
    out[1] = bus_read(bank, 1);
    out[2] = bus_read(bank, 0);
    out[3] = bus_read(bank, 1);
    bus_write(bank, 0, 0xF0);
    return out[0] == 0xBF && out[1] == 0xB7 &&
           out[2] == 0xBF && out[3] == 0xB7 ? OK : ERR_ID;
}

static uint8_t identified(uint8_t bank)
{
    uint8_t id[4];
    return flash_id(bank, id);
}

static uint8_t verify_error(uint32_t address, uint8_t expected, uint8_t actual)
{
    flash_error_address = address;
    flash_error_expected = expected;
    flash_error_actual = actual;
    return ERR_VERIFY;
}

/* Bounded DQ7 polling; compare complete data in two additional reads after
 * DQ7 matches. At the intended 1 MHz CPU clock the explicit delay budget is
 * 500 ms, plus bounded bus overhead (datasheet maximum erase time: 25 ms).
 */
static uint8_t wait_ready(uint8_t bank, uint32_t address, uint8_t expected)
{
    for (uint16_t tries = 0; tries < 5000; ++tries) {
        uint8_t value = bus_read(bank, address);
        if (!((value ^ expected) & 0x80)) {
            uint8_t a = bus_read(bank, address);
            uint8_t b = bus_read(bank, address);
            if (a == expected && b == expected) return OK;
            if (a == b && !((a ^ expected) & 0x80))
                return verify_error(address, expected, a);
        }
        bus_wait_100us();
    }
    flash_error_address = address;
    flash_error_expected = expected;
    flash_error_actual = bus_read(bank, address);
    return ERR_TIMEOUT;
}

uint8_t flash_read_block(uint8_t bank, uint32_t address, uint8_t *out, uint8_t length)
{
    if (!length || length > BLOCK_SIZE) return ERR_LENGTH;
    uint8_t status = flash_bounds(bank, address, length);
    if (status) return status;
    for (uint8_t i = 0; i < length; ++i) out[i] = bus_read(bank, address + i);
    return OK;
}

uint8_t flash_erase_sector(uint8_t bank, uint32_t address)
{
    uint8_t status = flash_bounds(bank, address, SECTOR_SIZE);
    if (status) return status;
    if (address & (SECTOR_SIZE - 1)) return ERR_RANGE;
    status = identified(bank);
    if (status) return status;
    unlock(bank);
    bus_write(bank, 0x5555, 0x80);
    unlock(bank);
    bus_write(bank, address, 0x30);
    status = wait_ready(bank, address, 0xFF);
    if (status) return status;
    for (uint16_t i = 0; i < SECTOR_SIZE; ++i) {
        uint8_t value = bus_read(bank, address + i);
        if (value != 0xFF) return verify_error(address + i, 0xFF, value);
    }
    return OK;
}

uint8_t flash_program(uint8_t bank, uint32_t address, const uint8_t *data, uint8_t length)
{
    if (!length || length > BLOCK_SIZE) return ERR_LENGTH;
    uint8_t status = flash_bounds(bank, address, length);
    if (status) return status;
    status = identified(bank);
    if (status) return status;
    /* Preflight the entire block before programming any byte. Flash cannot
     * change 0 back to 1 without erasing its sector first.
     */
    for (uint8_t i = 0; i < length; ++i) {
        uint8_t old = bus_read(bank, address + i);
        if ((old & data[i]) != data[i]) {
            flash_error_address = address + i;
            flash_error_expected = data[i];
            flash_error_actual = old;
            return ERR_NEEDS_ERASE;
        }
    }
    for (uint8_t i = 0; i < length; ++i) {
        uint32_t at = address + i;
        if (bus_read(bank, at) == data[i]) continue;
        unlock(bank);
        bus_write(bank, 0x5555, 0xA0);
        bus_write(bank, at, data[i]);
        status = wait_ready(bank, at, data[i]);
        if (status) return status;
    }
    for (uint8_t i = 0; i < length; ++i) {
        uint8_t value = bus_read(bank, address + i);
        if (value != data[i]) return verify_error(address + i, data[i], value);
    }
    return OK;
}
