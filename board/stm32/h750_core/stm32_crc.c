/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#include "tx_user.h"

uint32_t _crc32(uint32_t c, uint32_t poly, uint32_t polysize, const uint8_t *p, size_t size) {
    uint32_t cr = CRC->CR;

    cr = (cr & ~CRC_CR_POLYSIZE) | polysize | CRC_CR_RESET;
    CRC->CR = cr;
    CRC->POL = poly;
    CRC->INIT = c;
    while (size > 0) {
        CRC->DR = *p++;
        size -= sizeof(*p);
    }
    return CRC->DR;
}

#define crc_generate(_type)