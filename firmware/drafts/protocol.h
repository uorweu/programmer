#ifndef SST_PROTOCOL_H
#define SST_PROTOCOL_H
#include <stdint.h>
#define PROTOCOL_VERSION 1
#define HEADER_SIZE 8
enum { CMD_PING = 1, CMD_ID = 2, CMD_READ = 3, CMD_ERASE = 4, CMD_WRITE = 5 };
/* Header: version, command, sequence, bank, address LE24, payload length.
 * out has BLOCK_SIZE+1 bytes; first response byte is always status.
 */
uint8_t protocol_handle(const uint8_t header[HEADER_SIZE], const uint8_t *payload, uint8_t *out);
#endif
