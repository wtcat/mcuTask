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


    .text
    .align 3
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_context_save                           Cortex-A35/AC6    */
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Andres Mlinar, Microsoft Corporation                                */
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
/*  12-31-2020     Andres Mlinar            Initial Version 6.1.3         */
/*                                                                        */
/**************************************************************************/
// VOID   _tx_thread_context_save(VOID)
// {
    .global _tx_thread_context_save
    .type   _tx_thread_context_save, @function
_tx_thread_context_save:

    /* Upon entry to this routine, it is assumed that IRQ/FIQ interrupts are locked
       out, x29 (frame pointer), x30 (link register) are saved, we are in EL1, 
       and all other registers are intact.  */

    /* Check for a nested interrupt condition.  */
    // if (_tx_thread_system_state++)
    // {

    STP     x0, x1, [sp, #-16]!                 // Save x0, x1
    STP     x2, x3, [sp, #-16]!                 // Save x2, x3
    LDR     x3, =_tx_thread_system_state        // Pickup address of system state var
    LDR     w2, [x3, #0]                        // Pickup system state
    CMP     w2, #0                              // Is this the first interrupt?
    BEQ     __tx_thread_not_nested_save         // Yes, not a nested context save

    /* Nested interrupt condition.  */

    ADD     w2, w2, #1                          // Increment the nested interrupt counter
    STR     w2, [x3, #0]                        // Store it back in the variable

   /* Save the rest of the scratch registers on the stack and return to the
      calling ISR.  */

    STP     x4, x5, [sp, #-16]!                 // Save x4, x5
    STP     x6, x7, [sp, #-16]!                 // Save x6, x7
    STP     x8, x9, [sp, #-16]!                 // Save x8, x9
    STP     x10, x11, [sp, #-16]!               // Save x10, x11
    STP     x12, x13, [sp, #-16]!               // Save x12, x13
    STP     x14, x15, [sp, #-16]!               // Save x14, x15
    STP     x16, x17, [sp, #-16]!               // Save x16, x17
    STP     x18, x19, [sp, #-16]!               // Save x18, x19
#ifdef EL1
    MRS     x0, SPSR_EL1                        // Pickup SPSR
    MRS     x1, ELR_EL1                         // Pickup ELR (point of interrupt)
#else
#ifdef EL2
    MRS     x0, SPSR_EL2                        // Pickup SPSR
    MRS     x1, ELR_EL2                         // Pickup ELR (point of interrupt)
#else
    MRS     x0, SPSR_EL3                        // Pickup SPSR
    MRS     x1, ELR_EL3                         // Pickup ELR (point of interrupt)
#endif
#endif
    STP     x0, x1, [sp, #-16]!                 // Save SPSR, ELR

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY

    /* Call the ISR enter function to indicate an ISR is executing.  */

    STP     x29, x30, [sp, #-16]!               // Save x29, x30
    BL      _tx_execution_isr_enter             // Call the ISR enter function
    LDP     x29, x30, [sp], #16                 // Recover x29, x30
#endif

    /* Return to the ISR.  */

    RET                                         // Return to ISR

__tx_thread_not_nested_save:
    // }

    /* Otherwise, not nested, check to see if a thread was running.  */
    // else if (_tx_thread_current_ptr)
    // {

    ADD     w2, w2, #1                          // Increment the interrupt counter
    STR     w2, [x3, #0]                        // Store it back in the variable
    LDR     x1, =_tx_thread_current_ptr         // Pickup address of current thread ptr
    LDR     x0, [x1, #0]                        // Pickup current thread pointer
    CMP     x0, #0                              // Is it NULL?
    BEQ     __tx_thread_idle_system_save        // If so, interrupt occurred in 
                                                //   scheduling loop - nothing needs saving!

    /* Save minimal context of interrupted thread.  */

    STP     x4, x5, [sp, #-16]!                 // Save x4, x5
    STP     x6, x7, [sp, #-16]!                 // Save x6, x7
    STP     x8, x9, [sp, #-16]!                 // Save x8, x9
    STP     x10, x11, [sp, #-16]!               // Save x10, x11
    STP     x12, x13, [sp, #-16]!               // Save x12, x13
    STP     x14, x15, [sp, #-16]!               // Save x14, x15
    STP     x16, x17, [sp, #-16]!               // Save x16, x17
    STP     x18, x19, [sp, #-16]!               // Save x18, x19
#ifdef EL1
    MRS     x4, SPSR_EL1                        // Pickup SPSR
    MRS     x5, ELR_EL1                         // Pickup ELR (point of interrupt)
#else
#ifdef EL2
    MRS     x4, SPSR_EL2                        // Pickup SPSR
    MRS     x5, ELR_EL2                         // Pickup ELR (point of interrupt)
#else
    MRS     x4, SPSR_EL3                        // Pickup SPSR
    MRS     x5, ELR_EL3                         // Pickup ELR (point of interrupt)
#endif
#endif
    STP     x4, x5, [sp, #-16]!                 // Save SPSR, ELR

    /* Save the current stack pointer in the thread's control block.  */
    // _tx_thread_current_ptr -> tx_thread_stack_ptr =  sp;

    MOV     x4, sp                              //
    STR     x4, [x0, #8]                        // Save thread stack pointer

    /* Switch to the system stack.  */
    // sp =  _tx_thread_system_stack_ptr

    LDR     x3, =_tx_thread_system_stack_ptr    // Pickup address of system stack
    LDR     x4, [x3, #0]                        // Pickup system stack pointer
    MOV     sp, x4                              // Setup system stack pointer

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY

    /* Call the ISR enter function to indicate an ISR is executing.  */

    STP     x29, x30, [sp, #-16]!               // Save x29, x30
    BL      _tx_execution_isr_enter             // Call the ISR enter function
    LDP     x29, x30, [sp], #16                 // Recover x29, x30
#endif

    RET                                         // Return to caller

    // }
    // else
    // {

__tx_thread_idle_system_save:

    /* Interrupt occurred in the scheduling loop.  */

    /* Not much to do here, just adjust the stack pointer, and return to IRQ 
       processing.  */

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY

    /* Call the ISR enter function to indicate an ISR is executing.  */

    STP     x29, x30, [sp, #-16]!               // Save x29, x30
    BL      _tx_execution_isr_enter             // Call the ISR enter function
    LDP     x29, x30, [sp], #16                 // Recover x29, x30
#endif

    ADD     sp, sp, #48                         // Recover saved registers
    RET                                         // Continue IRQ processing  

    // }
// } 

