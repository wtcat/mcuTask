/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#include "tx_api.h"


 uint32_t stm32_crc32(uint32_t poly, uint32_t polysize, uint32_t c, 
    uint8_t *p, size_t size) {
    uint32_t cr = CRC->CR;
    CRC->CR   = (cr & ~CRC_CR_POLYSIZE) | polysize;
    CRC->POL  = poly;
    CRC->INIT = c;
    CRC->CR  |= CRC_CR_RESET;
    
    long count = (long)rte_div_roundup(size, 8);
    switch (size & 7) {
    case 0: do{ *(volatile typeof(p))&CRC->DR = *p++;
    case 7: *(volatile typeof(p))&CRC->DR = *p++;
    case 6: *(volatile typeof(p))&CRC->DR = *p++;
    case 5: *(volatile typeof(p))&CRC->DR = *p++;
    case 4: *(volatile typeof(p))&CRC->DR = *p++;
    case 3: *(volatile typeof(p))&CRC->DR = *p++;
    case 2: *(volatile typeof(p))&CRC->DR = *p++;
    case 1: *(volatile typeof(p))&CRC->DR = *p++;
            } while (--count > 0);
    }
    return CRC->DR;
}


