#include "Crc16.hpp"

using namespace Common;

Crc16::Crc16()
{}

uint16_t Crc16::Calculate(uint16_t checksum, uint8_t *buffer, int length)
{
    for (int i = 0;  i < length;  i++)
        checksum = crc16Table[((checksum >> 8) ^ *buffer++) & 0xff] ^ (checksum << 8);

    return checksum;
}
