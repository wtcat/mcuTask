/*
 * Copyright (c) 2024 wtcat
 */
// #define TX_SOURCE_CODE

#include "tx_api.h"
// #include "tx_trace.h"
// #include "tx_thread.h"


#if 0
static void nanosleep_timeout(void) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
#ifdef TX_NOT_INTERRUPTABLE
    /* Resume the thread!  */
    _tx_thread_system_ni_resume(&xxxxxx);
    TX_RESTORE
#else
    _tx_thread_preempt_disable++;
    TX_RESTORE
    _tx_thread_system_resume(&_tx_timer_thread);
#endif
}

UINT tx_os_nanosleep(uint64_t time) {
    TX_INTERRUPT_SAVE_AREA
    TX_THREAD *thread_ptr;

    /* Lockout interrupts while the thread is being resumed.  */
    TX_DISABLE
    TX_THREAD_GET_CURRENT(thread_ptr)

    /* Is there a current thread?  */
    if (thread_ptr == TX_NULL) {
        TX_RESTORE
        return TX_CALLER_ERROR;
    }

    /* Is the caller an ISR or Initialization?  */
    if (TX_THREAD_GET_SYSTEM_STATE() != ((ULONG) 0)) {
        TX_RESTORE
        return TX_CALLER_ERROR;
    }

    if (time == 0) {
        TX_RESTORE
        return TX_SUCCESS;
    }

    /* Determine if the preempt disable flag is non-zero.  */
    if (_tx_thread_preempt_disable != ((UINT) 0)) {
        TX_RESTORE
        return TX_CALLER_ERROR;
    }

    /* If trace is enabled, insert this event into the trace buffer.  */
    TX_TRACE_IN_LINE_INSERT(TX_TRACE_THREAD_SLEEP, TX_ULONG_TO_POINTER_CONVERT(timer_ticks), 
        thread_ptr->tx_thread_state, TX_POINTER_TO_ULONG_CONVERT(&status), 
        0, TX_TRACE_THREAD_EVENTS)

    /* Log this kernel call.  */
    TX_EL_THREAD_SLEEP_INSERT

    /* Suspend the current thread.  */
    thread_ptr->tx_thread_state = TX_SUSPENDED;

#ifdef TX_NOT_INTERRUPTABLE
    /* Call actual non-interruptable thread suspension routine.  */
    _tx_thread_system_ni_suspend(thread_ptr, TX_NO_WAIT);
    //TODO:
    TX_RESTORE
#else
    thread_ptr->tx_thread_suspending =  TX_TRUE;
    thread_ptr->tx_thread_suspend_status =  TX_SUCCESS;
    thread_ptr->tx_thread_timer.tx_timer_internal_remaining_ticks = TX_NO_WAIT;
    _tx_thread_preempt_disable++;
    //TODO:
    TX_RESTORE
    _tx_thread_system_suspend(thread_ptr);
#endif

    /* Return status to the caller.  */
    return thread_ptr->tx_thread_suspend_status;
}
#else
UINT tx_os_nanosleep(uint64_t time) {
    ULONG ms = time / 1000000 + 1;
    return tx_thread_sleep(TX_MSEC(ms));
}
#endif /* #if 0*/

UINT tx_os_delay(uint64_t nano_sec) {
    return 0;
}
