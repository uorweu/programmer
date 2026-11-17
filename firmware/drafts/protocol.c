#include "protocol.h"
#include "flash.h"

uint8_t protocol_handle(const uint8_t *h, const uint8_t *payload, uint8_t *out)
{
    uint32_t address = (uint32_t)h[4] | ((uint32_t)h[5] << 8) | ((uint32_t)h[6] << 16);
    uint8_t command = h[1], bank = h[3], length = h[7];
    out[0] = OK;
    flash_error_address = address;
    flash_error_expected = 0;
    flash_error_actual = 0;
    if (h[0] != PROTOCOL_VERSION) out[0] = ERR_VERSION;
    else if (length > BLOCK_SIZE) out[0] = ERR_LENGTH;
    else if (bank > 1) out[0] = ERR_BANK;
    else if (command == CMD_PING) {
        if (length || address) out[0] = ERR_LENGTH;
        else {
            out[1] = 'S'; out[2] = 'S'; out[3] = 'T'; out[4] = '1';
            out[5] = BLOCK_SIZE;
            return 6;
        }
    } else if (command == CMD_ID) {
        if (length || address) out[0] = ERR_LENGTH;
        else { out[0] = flash_id(bank, out + 1); return 5; }
    } else if (command == CMD_READ) {
        if (length != 1 || !payload[0] || payload[0] > BLOCK_SIZE)
            out[0] = ERR_LENGTH;
        else {
            out[0] = flash_read_block(bank, address, out + 1, payload[0]);
            if (!out[0]) return payload[0] + 1;
        }
    } else if (command == CMD_ERASE) {
        if (length != 5 || payload[0] != 'E' || payload[1] != 'R' ||
            payload[2] != 'A' || payload[3] != 'S' || payload[4] != 'E')
            out[0] = ERR_CONFIRM;
        else out[0] = flash_erase_sector(bank, address);
    } else if (command == CMD_WRITE) {
        out[0] = flash_program(bank, address, payload, length);
    } else out[0] = ERR_COMMAND;

    if (out[0] == ERR_VERIFY || out[0] == ERR_NEEDS_ERASE || out[0] == ERR_TIMEOUT) {
        out[1] = (uint8_t)flash_error_address;
        out[2] = (uint8_t)(flash_error_address >> 8);
        out[3] = (uint8_t)(flash_error_address >> 16);
        out[4] = flash_error_expected;
        out[5] = flash_error_actual;
        return 6;
    }
    return 1;
}
