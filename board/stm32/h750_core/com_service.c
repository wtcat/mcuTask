/*
 * Copyright 2024 wtat
 */

#include "tx_api.h"

#define MAX_MTU 512
#define MIN_FRAME_SIZE 9


struct link_header {
    uint8_t  dst_addr;
    uint8_t  src_addr;
    uint8_t  seq_num;
    uint16_t len;
} __rte_packed;


static void recv_service(void *arg) {

}


