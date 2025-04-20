/*
 * Copyright 2025 wtcat
 */
#define TX_SOURCE_CODE
#include <tx_api.h>
#include <tx_thread.h>

#include <base/generic.h>

#define _STATE_ITEM(_name) [TX_##_name] = #_name
static const char *thread_state_name[] = {
	_STATE_ITEM(READY),
	_STATE_ITEM(COMPLETED),
	_STATE_ITEM(TERMINATED),
	_STATE_ITEM(SUSPENDED),
	_STATE_ITEM(SLEEP),
	_STATE_ITEM(QUEUE_SUSP),
	_STATE_ITEM(SEMAPHORE_SUSP),
	_STATE_ITEM(EVENT_FLAG),
	_STATE_ITEM(BLOCK_MEMORY),
	_STATE_ITEM(BYTE_MEMORY),
	_STATE_ITEM(IO_DRIVER),
	_STATE_ITEM(FILE),
	_STATE_ITEM(TCP_IP),
	_STATE_ITEM(MUTEX_SUSP),
	_STATE_ITEM(PRIORITY_CHANGE),
};

const char *tx_thread_state_name(UINT state) {
    if (state < rte_array_size(thread_state_name))
        return thread_state_name[state];
    return "Invalid";
}

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
