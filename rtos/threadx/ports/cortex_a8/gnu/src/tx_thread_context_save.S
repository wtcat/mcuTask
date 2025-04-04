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
/**   Thread                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/
#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif

    .syntax unified
#if defined(THUMB_MODE)
    .thumb
#else
    .arm
#endif

    .global     _tx_thread_system_state
    .global     _tx_thread_current_ptr
    .global     __tx_irq_processing_return


/* No 16-bit Thumb mode veneer code is needed for _tx_thread_context_save
   since it will never be called 16-bit mode.  */

    .text
    .align 2
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_context_save                              ARMv7-A        */
/*                                                           6.4.0        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function saves the context of an executing thread in the       */
/*    beginning of interrupt processing.  The function also ensures that  */
/*    the system stack is used upon return to the calling ISR.            */
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
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ISRs                                                                */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
/*  10-15-2021     William E. Lamie         Modified comment(s), added    */
/*                                            execution profile support,  */
/*                                            resulting in version 6.1.9  */
/*  04-25-2022     Zhen Kong                Updated comments,             */
/*                                            resulting in version 6.1.11 */
/*  10-31-2023     Tiejun Zhou              Modified comment(s), added    */
/*                                            #include tx_user.h,         */
/*                                            resulting in version 6.3.0  */
/*  12-31-2023     Yajun Xia                Modified comment(s),          */
/*                                            Added thumb mode support,   */
/*                                            resulting in version 6.4.0  */
/*                                                                        */
/**************************************************************************/
    .global _tx_thread_context_save
    .type   _tx_thread_context_save,function
_tx_thread_context_save:

    /* Upon entry to this routine, it is assumed that IRQ interrupts are locked
       out, we are in IRQ mode, and all registers are intact.  */

    /* Check for a nested interrupt condition.  */

    PUSH    {r0-r3}                         // Save some working registers
#ifdef TX_ENABLE_FIQ_SUPPORT
    CPSID   if                              // Disable FIQ interrupts
#endif
    LDR     r3, =_tx_thread_system_state    // Pickup address of system state variable
    LDR     r2, [r3]                        // Pickup system state
    CMP     r2, #0                          // Is this the first interrupt?
    BEQ     __tx_thread_not_nested_save     // Yes, not a nested context save

    /* Nested interrupt condition.  */

    ADD     r2, #1                          // Increment the interrupt counter
    STR     r2, [r3]                        // Store it back in the variable

   /* Save the rest of the scratch registers on the stack and return to the
      calling ISR.  */

    MRS     r0, SPSR                        // Pickup saved SPSR
    SUB     lr, #4                          // Adjust point of interrupt
    PUSH    {r0, r10, r12, lr}              // Store other registers

    /* Return to the ISR.  */

    MOV     r10, #0                         // Clear stack limit

#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))

    /* Call the ISR enter function to indicate an ISR is executing.  */

    PUSH    {lr}                            // Save ISR lr
    BL      _tx_execution_isr_enter         // Call the ISR enter function
    POP     {lr}                            // Recover ISR lr
#endif

    B       __tx_irq_processing_return      // Continue IRQ processing

__tx_thread_not_nested_save:

    /* Otherwise, not nested, check to see if a thread was running.  */
    ADD     r2, #1                          // Increment the interrupt counter
    STR     r2, [r3]                        // Store it back in the variable
    LDR     r1, =_tx_thread_current_ptr     // Pickup address of current thread ptr
    LDR     r0, [r1]                        // Pickup current thread pointer
    CMP     r0, #0                          // Is it NULL?
    BEQ     __tx_thread_idle_system_save    // If so, interrupt occurred in
                                            //   scheduling loop - nothing needs saving!

    /* Save minimal context of interrupted thread.  */

    MRS     r2, SPSR                        // Pickup saved SPSR
    SUB     lr, #4                          // Adjust point of interrupt
    PUSH    {r2, r10, r12, lr}              // Store other registers

    MOV     r10, #0                         // Clear stack limit

#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))

    /* Call the ISR enter function to indicate an ISR is executing.  */

    PUSH    {lr}                            // Save ISR lr
    BL      _tx_execution_isr_enter         // Call the ISR enter function
    POP     {lr}                            // Recover ISR lr
#endif

    B       __tx_irq_processing_return      // Continue IRQ processing

__tx_thread_idle_system_save:

    /* Interrupt occurred in the scheduling loop.  */

    /* Not much to do here, just adjust the stack pointer, and return to IRQ
       processing.  */

    MOV     r10, #0                         // Clear stack limit

#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))

    /* Call the ISR enter function to indicate an ISR is executing.  */

    PUSH    {lr}                            // Save ISR lr
    BL      _tx_execution_isr_enter         // Call the ISR enter function
    POP     {lr}                            // Recover ISR lr
#endif

    ADD     sp, #16                         // Recover saved registers
    B       __tx_irq_processing_return      // Continue IRQ processing
