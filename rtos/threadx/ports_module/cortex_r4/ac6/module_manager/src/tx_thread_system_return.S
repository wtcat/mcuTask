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

/* #define TX_SOURCE_CODE  */


/* Include necessary system files.  */

/* #include "tx_api.h" */
/* #include "tx_thread.h" */
/* #include "tx_timer.h" */


    .global      _tx_thread_current_ptr
    .global      _tx_timer_time_slice
    .global      _tx_thread_schedule
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
    .global      _tx_execution_thread_exit
#endif


#ifdef TX_THUMB_MODE
    .thumb
#else
    .arm
#endif
    .text
    .eabi_attribute Tag_ABI_align_preserved, 1
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_system_return                          Cortex-R4/AC6     */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is target processor specific.  It is used to transfer */
/*    control from a thread back to the ThreadX system.  Only a           */
/*    minimal context is saved since the compiler assumes temp registers  */
/*    are going to get slicked by a function call anyway.                 */
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
/*    _tx_thread_schedule                   Thread scheduling loop        */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ThreadX components                                                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
/*                                                                        */
/**************************************************************************/
/* VOID   _tx_thread_system_return(VOID) */
/* { */
    .global _tx_thread_system_return
    .type   _tx_thread_system_return, "function"
_tx_thread_system_return:

    /* Save minimal context on the stack.  */

    STMDB   sp!, {r4-r11, lr}                   // Save minimal context
    LDR     r5, =_tx_thread_current_ptr         // Pickup address of current ptr
    LDR     r6, [r5, #0]                        // Pickup current thread pointer

#ifdef __ARM_FP
    LDR     r0, [r6, #144]                      // Pickup the VFP enabled flag
    CMP     r0, #0                              // Is the VFP enabled?
    BEQ     _tx_skip_solicited_vfp_save         // No, skip VFP solicited save
    VMRS    r4, FPSCR                           // Pickup the FPSCR
    STR     r4, [sp, #-4]!                      // Save FPSCR
    VSTMDB  sp!, {D8-D15}                       // Save D8-D15
_tx_skip_solicited_vfp_save:
#endif

    MOV     r0, #0                              // Build a solicited stack type
    MRS     r1, CPSR                            // Pickup the CPSR
    STMDB   sp!, {r0-r1}                        // Save type and CPSR

   /* Lockout interrupts.  */

#ifdef TX_ENABLE_FIQ_SUPPORT
    CPSID   if                                  // Disable IRQ and FIQ interrupts
#else
    CPSID   i                                   // Disable IRQ interrupts
#endif

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY

    /* Call the thread exit function to indicate the thread is no longer executing.  */

    BL      _tx_execution_thread_exit           // Call the thread exit function
#endif
    LDR     r2, =_tx_timer_time_slice           // Pickup address of time slice
    LDR     r1, [r2, #0]                        // Pickup current time slice

    /* Save current stack and switch to system stack.  */
    /* _tx_thread_current_ptr -> tx_thread_stack_ptr =  sp; */
    /* sp = _tx_thread_system_stack_ptr; */

    STR     sp, [r6, #8]                        // Save thread stack pointer

    /* Determine if the time-slice is active.  */
    /* if (_tx_timer_time_slice) */
    /* { */

    MOV     r4, #0                              // Build clear value
    CMP     r1, #0                              // Is a time-slice active?
    BEQ     __tx_thread_dont_save_ts            // No, don't save the time-slice

       /* Save the current remaining time-slice.  */
       /* _tx_thread_current_ptr -> tx_thread_time_slice =  _tx_timer_time_slice; */
       /* _tx_timer_time_slice =  0; */

    STR     r4, [r2, #0]                        // Clear time-slice
    STR     r1, [r6, #24]                       // Store current time-slice

    /* } */
__tx_thread_dont_save_ts:

    /* Clear the current thread pointer.  */
    /* _tx_thread_current_ptr =  TX_NULL; */

    STR     r4, [r5, #0]                        // Clear current thread pointer

    B       _tx_thread_schedule                 // Jump to scheduler!

/* } */
