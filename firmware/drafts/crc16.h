#ifndef SST_CRC16_H
#define SST_CRC16_H
#include <stdint.h>
static inline uint16_t crc16_byte(uint16_t crc, uint8_t value)
{
    crc ^= (uint16_t)value << 8;
    for (uint8_t bit = 0; bit < 8; ++bit)
        crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
    return crc;
}
#endif
