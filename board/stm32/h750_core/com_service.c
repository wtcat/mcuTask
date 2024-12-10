/*
 * Copyright 2024 wtat
 */

#include "tx_api.h"

#define MAX_MTU 512

struct sproto_header {
    uint8_t  dst_addr;
    uint8_t  src_addr;
    uint16_t op_code;
    uint16_t len;
} __rte_packed;



