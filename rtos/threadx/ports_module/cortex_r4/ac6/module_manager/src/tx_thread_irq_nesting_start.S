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

#define SYS_MODE        0x1F            // System mode bits

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
/*    _tx_thread_irq_nesting_start                      Cortex-R4/AC6     */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is called by the application from IRQ mode after      */
/*    _tx_thread_context_save has been called and switches the IRQ        */
/*    processing to the system mode so nested IRQ interrupt processing    */
/*    is possible (system mode has its own "lr" register).  Note that     */
/*    this function assumes that the system mode stack pointer was setup  */
/*    during low-level initialization (tx_initialize_low_level.s).        */
/*                                                                        */
/*    This function returns with IRQ interrupts enabled.                  */
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
/*                                                                        */
/**************************************************************************/
/* VOID   _tx_thread_irq_nesting_start(VOID) */
/* { */
    .global _tx_thread_irq_nesting_start
    .type   _tx_thread_irq_nesting_start, "function"
_tx_thread_irq_nesting_start:
    MOV     r3, lr                              // Save ISR return address
    CPS     #SYS_MODE                           // Switch to System Mode
    STMDB   sp!, {r1, lr}                       // Push the system mode lr on the system mode stack
                                                //   and push r1 just to keep 8-byte alignment
    CPSIE   i                                   // Enable IRQ interrupts
    BX      r3                                  // Return to caller
/* } */
