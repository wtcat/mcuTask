/*
 * Copyright 2025 wtcat
 */

#include "tx_api.h"

__externC int main(void) {

    while (1)
        tx_thread_sleep(TX_MSEC(1000));

    return 0;
}