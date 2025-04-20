/*
 * Copyright 2025 wtcat
 */
#define TX_SOURCE_CODE
#include <tx_api.h>
#include <tx_thread.h>

#include <base/compiler.h>

void tx_thread_foreach(bool (*iterator)(TX_THREAD *, void *arg), void *arg) {
    TX_INTERRUPT_SAVE_AREA
    TX_THREAD *thread_ptr;
    UINT thread_count;

    TX_DISABLE
    _tx_thread_preempt_disable++;
    TX_RESTORE

    thread_ptr   = _tx_thread_created_ptr;
    thread_count = _tx_thread_created_count;

    while (thread_count > 0) {
        if (iterator(thread_ptr, arg))
            break;

        thread_count--;

        /* Pointer to the next thread */
        thread_ptr = thread_ptr->tx_thread_created_next;
    }
    
    TX_DISABLE
    _tx_thread_preempt_disable--;
    TX_RESTORE
}
