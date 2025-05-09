@/***************************************************************************
@ * Copyright (c) 2024 Microsoft Corporation 
@ * 
@ * This program and the accompanying materials are made available under the
@ * terms of the MIT License which is available at
@ * https://opensource.org/licenses/MIT.
@ * 
@ * SPDX-License-Identifier: MIT
@ **************************************************************************/
@
@
@/**************************************************************************/
@/**************************************************************************/
@/**                                                                       */ 
@/** ThreadX Component                                                     */ 
@/**                                                                       */
@/**   Thread                                                              */
@/**                                                                       */
@/**************************************************************************/
@/**************************************************************************/
#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif

#ifdef TX_ENABLE_FIQ_SUPPORT
DISABLE_INTS    =       0xC0                    @ Disable IRQ/FIQ interrupts
#else
DISABLE_INTS    =       0x80                    @ Disable IRQ interrupts
#endif
MODE_MASK       =       0x1F                    @ Mode mask 
FIQ_MODE_BITS   =       0x11                    @ FIQ mode bits
@
@
@/* No 16-bit Thumb mode veneer code is needed for _tx_thread_fiq_nesting_end
@   since it will never be called 16-bit mode.  */
@
    .arm
    .text
    .align 2
@/**************************************************************************/ 
@/*                                                                        */ 
@/*  FUNCTION                                               RELEASE        */ 
@/*                                                                        */ 
@/*    _tx_thread_fiq_nesting_end                        Cortex-R5/GNU     */ 
@/*                                                           6.1          */
@/*  AUTHOR                                                                */
@/*                                                                        */
@/*    William E. Lamie, Microsoft Corporation                             */
@/*                                                                        */
@/*  DESCRIPTION                                                           */
@/*                                                                        */ 
@/*    This function is called by the application from FIQ mode after      */ 
@/*    _tx_thread_fiq_nesting_start has been called and switches the FIQ   */ 
@/*    processing from system mode back to FIQ mode prior to the ISR       */ 
@/*    calling _tx_thread_fiq_context_restore.  Note that this function    */ 
@/*    assumes the system stack pointer is in the same position after      */ 
@/*    nesting start function was called.                                  */ 
@/*                                                                        */ 
@/*    This function assumes that the system mode stack pointer was setup  */ 
@/*    during low-level initialization (tx_initialize_low_level.s).        */ 
@/*                                                                        */ 
@/*    This function returns with FIQ interrupts disabled.                 */ 
@/*                                                                        */ 
@/*  INPUT                                                                 */ 
@/*                                                                        */ 
@/*    None                                                                */ 
@/*                                                                        */ 
@/*  OUTPUT                                                                */ 
@/*                                                                        */ 
@/*    None                                                                */ 
@/*                                                                        */ 
@/*  CALLS                                                                 */ 
@/*                                                                        */ 
@/*    None                                                                */ 
@/*                                                                        */ 
@/*  CALLED BY                                                             */ 
@/*                                                                        */ 
@/*    ISRs                                                                */ 
@/*                                                                        */ 
@/*  RELEASE HISTORY                                                       */ 
@/*                                                                        */ 
@/*    DATE              NAME                      DESCRIPTION             */
@/*                                                                        */
@/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
@/*                                                                        */
@/**************************************************************************/
@VOID   _tx_thread_fiq_nesting_end(VOID)
@{
    .global  _tx_thread_fiq_nesting_end
    .type    _tx_thread_fiq_nesting_end,function
_tx_thread_fiq_nesting_end:
    MOV     r3,lr                               @ Save ISR return address
    MRS     r0, CPSR                            @ Pickup the CPSR
    ORR     r0, r0, #DISABLE_INTS               @ Build disable interrupt value
    MSR     CPSR_c, r0                          @ Disable interrupts
    LDMIA   sp!, {r1, lr}                       @ Pickup saved lr (and r1 throw-away for 
                                                @   8-byte alignment logic)
    BIC     r0, r0, #MODE_MASK                  @ Clear mode bits
    ORR     r0, r0, #FIQ_MODE_BITS              @ Build IRQ mode CPSR
    MSR     CPSR_c, r0                          @ Reenter IRQ mode

#ifdef __THUMB_INTERWORK
    BX      r3                                  @ Return to caller
#else
    MOV     pc, r3                              @ Return to caller
#endif
@}

