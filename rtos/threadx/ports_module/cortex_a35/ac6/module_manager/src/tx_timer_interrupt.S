/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** ThreadX Component                                                     */
/**                                                                       */
/**   Timer                                                               */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


    .text
    .align 3
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_timer_interrupt                               Cortex-A35/AC6    */
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Andres Mlinar, Microsoft Corporation                                */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function processes the hardware timer interrupt.  This         */
/*    processing includes incrementing the system clock and checking for  */
/*    time slice and/or timer expiration.  If either is found, the        */
/*    interrupt context save/restore functions are called along with the  */
/*    expiration functions.                                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    _tx_timer_expiration_process          Timer expiration processing   */
/*    _tx_thread_time_slice                 Time slice interrupted thread */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    interrupt vector                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  12-31-2020     Andres Mlinar            Initial Version 6.1.3         */
/*                                                                        */
/**************************************************************************/
// VOID   _tx_timer_interrupt(VOID)
// {
    .global _tx_timer_interrupt
    .type   _tx_timer_interrupt, @function
_tx_timer_interrupt:

    /* Upon entry to this routine, it is assumed that context save has already
       been called, and therefore the compiler scratch registers are available
       for use.  */

    /* Increment the system clock.  */
    // _tx_timer_system_clock++;

    LDR     x1, =_tx_timer_system_clock         // Pickup address of system clock
    LDR     w0, [x1, #0]                        // Pickup system clock
    ADD     w0, w0, #1                          // Increment system clock
    STR     w0, [x1, #0]                        // Store new system clock

    /* Test for time-slice expiration.  */
    // if (_tx_timer_time_slice)
    // {

    LDR     x3, =_tx_timer_time_slice           // Pickup address of time-slice 
    LDR     w2, [x3, #0]                        // Pickup time-slice
    CMP     w2, #0                              // Is it non-active?
    BEQ     __tx_timer_no_time_slice            // Yes, skip time-slice processing

       /* Decrement the time_slice.  */
       // _tx_timer_time_slice--;

    SUB     w2, w2, #1                          // Decrement the time-slice
    STR     w2, [x3, #0]                        // Store new time-slice value

       /* Check for expiration.  */
       // if (__tx_timer_time_slice == 0)

    CMP     w2, #0                              // Has it expired?
    BNE     __tx_timer_no_time_slice            // No, skip expiration processing

       /* Set the time-slice expired flag.  */
       // _tx_timer_expired_time_slice =  TX_TRUE;

    LDR     x3, =_tx_timer_expired_time_slice   // Pickup address of expired flag
    MOV     w0, #1                              // Build expired value
    STR     w0, [x3, #0]                        // Set time-slice expiration flag

    /* } */

__tx_timer_no_time_slice:

    /* Test for timer expiration.  */
    // if (*_tx_timer_current_ptr)
    // {

    LDR     x1, =_tx_timer_current_ptr          // Pickup current timer pointer addr
    LDR     x0, [x1, #0]                        // Pickup current timer
    LDR     x2, [x0, #0]                        // Pickup timer list entry
    CMP     x2, #0                              // Is there anything in the list?
    BEQ     __tx_timer_no_timer                 // No, just increment the timer

        /* Set expiration flag.  */
        // _tx_timer_expired =  TX_TRUE;

    LDR     x3, =_tx_timer_expired              // Pickup expiration flag address
    MOV     w2, #1                              // Build expired value
    STR     w2, [x3, #0]                        // Set expired flag
    B       __tx_timer_done                     // Finished timer processing

    // }
    // else
    // {
__tx_timer_no_timer:

        /* No timer expired, increment the timer pointer.  */
        // _tx_timer_current_ptr++;

    ADD     x0, x0, #8                          // Move to next timer

        /* Check for wrap-around.  */
        // if (_tx_timer_current_ptr == _tx_timer_list_end)

    LDR     x3, =_tx_timer_list_end             // Pickup addr of timer list end
    LDR     x2, [x3, #0]                        // Pickup list end
    CMP     x0, x2                              // Are we at list end?
    BNE     __tx_timer_skip_wrap                // No, skip wrap-around logic

            /* Wrap to beginning of list.  */
            // _tx_timer_current_ptr =  _tx_timer_list_start;

    LDR     x3, =_tx_timer_list_start           // Pickup addr of timer list start
    LDR     x0, [x3, #0]                        // Set current pointer to list start

__tx_timer_skip_wrap:

    STR     x0, [x1, #0]                        // Store new current timer pointer
    // }

__tx_timer_done:


    /* See if anything has expired.  */
    // if ((_tx_timer_expired_time_slice) || (_tx_timer_expired))
    // {

    LDR     x3, =_tx_timer_expired_time_slice   // Pickup addr of expired flag
    LDR     w2, [x3, #0]                        // Pickup time-slice expired flag
    CMP     w2, #0                              // Did a time-slice expire?
    BNE     __tx_something_expired              // If non-zero, time-slice expired
    LDR     x1, =_tx_timer_expired              // Pickup addr of other expired flag
    LDR     w0, [x1, #0]                        // Pickup timer expired flag
    CMP     w0, #0                              // Did a timer expire?
    BEQ     __tx_timer_nothing_expired          // No, nothing expired

__tx_something_expired:


    STP     x29, x30, [sp, #-16]!               // Save x29 (frame pointer), x30 (link register)

    /* Did a timer expire?  */
    // if (_tx_timer_expired)
    // {

    LDR     x1, =_tx_timer_expired              // Pickup addr of expired flag
    LDR     w0, [x1, #0]                        // Pickup timer expired flag
    CMP     w0, #0                              // Check for timer expiration
    BEQ     __tx_timer_dont_activate            // If not set, skip timer activation

        /* Process timer expiration.  */
        // _tx_timer_expiration_process();

    BL      _tx_timer_expiration_process        // Call the timer expiration handling routine

    /* }  */
__tx_timer_dont_activate:

    /* Did time slice expire?  */
    // if (_tx_timer_expired_time_slice)
    // {

    LDR     x3, =_tx_timer_expired_time_slice   // Pickup addr of time-slice expired 
    LDR     w2, [x3, #0]                        // Pickup the actual flag
    CMP     w2, #0                              // See if the flag is set
    BEQ     __tx_timer_not_ts_expiration        // No, skip time-slice processing

        /* Time slice interrupted thread.  */
        // _tx_thread_time_slice();

    BL      _tx_thread_time_slice               // Call time-slice processing

    // }

__tx_timer_not_ts_expiration:

    LDP     x29, x30, [sp], #16                 // Recover x29, x30
    // }

__tx_timer_nothing_expired:

    RET                                         // Return to caller

// }
