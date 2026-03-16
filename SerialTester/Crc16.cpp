#include "Crc16.hpp"

using namespace Common;

Crc16::Crc16()
{}

uint16_t Crc16::Calculate(uint16_t cksum, uint8_t *buf, int len)
{
    for (int i = 0;  i < len;  i++)
        cksum = crc16_tab[((cksum>>8) ^ *buf++) & 0xff] ^ (cksum << 8);

    return cksum;
}