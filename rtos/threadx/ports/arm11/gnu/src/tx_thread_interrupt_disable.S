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
DISABLE_INTS    =           0xC0                @ IRQ & FIQ interrupts disabled
#else
DISABLE_INTS    =           0x80                @ IRQ interrupts disabled
#endif
@
@
@/* Define the 16-bit Thumb mode veneer for _tx_thread_interrupt_disable for
@   applications calling this function from to 16-bit Thumb mode.  */
@
    .text
    .align 2
    .global $_tx_thread_interrupt_disable
$_tx_thread_interrupt_disable:
        .thumb
     BX        pc                               @ Switch to 32-bit mode
     NOP                                        @ 
    .arm
     STMFD     sp!, {lr}                        @ Save return address
     BL        _tx_thread_interrupt_disable     @ Call _tx_thread_interrupt_disable function
     LDMFD     sp!, {lr}                        @ Recover saved return address
     BX        lr                               @ Return to 16-bit caller
@
@
    .text
    .align 2
@/**************************************************************************/ 
@/*                                                                        */ 
@/*  FUNCTION                                               RELEASE        */ 
@/*                                                                        */ 
@/*    _tx_thread_interrupt_disable                        ARM11/GNU       */ 
@/*                                                            6.2.1       */
@/*  AUTHOR                                                                */
@/*                                                                        */
@/*    William E. Lamie, Microsoft Corporation                             */
@/*                                                                        */
@/*  DESCRIPTION                                                           */
@/*                                                                        */ 
@/*    This function is responsible for disabling interrupts               */ 
@/*                                                                        */ 
@/*  INPUT                                                                 */ 
@/*                                                                        */ 
@/*    None                                                                */ 
@/*                                                                        */ 
@/*  OUTPUT                                                                */ 
@/*                                                                        */ 
@/*    old_posture                           Old interrupt lockout posture */ 
@/*                                                                        */ 
@/*  CALLS                                                                 */ 
@/*                                                                        */ 
@/*    None                                                                */ 
@/*                                                                        */ 
@/*  CALLED BY                                                             */ 
@/*                                                                        */ 
@/*    Application Code                                                    */ 
@/*                                                                        */ 
@/*  RELEASE HISTORY                                                       */ 
@/*                                                                        */ 
@/*    DATE              NAME                      DESCRIPTION             */
@/*                                                                        */
@/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
@/*  03-08-2023     Cindy Deng               Modified comment(s), added    */
@/*                                            #include tx_user.h,         */
@/*                                            resulting in version 6.2.1  */
@/*                                                                        */
@/**************************************************************************/
@UINT   _tx_thread_interrupt_disable(void)
@{
    .global _tx_thread_interrupt_disable
    .type   _tx_thread_interrupt_disable,function
_tx_thread_interrupt_disable:
@
@    /* Pickup current interrupt lockout posture.  */
@
    MRS     r0, CPSR                            @ Pickup current CPSR
@
@    /* Mask interrupts.  */
@
    ORR     r1, r0, #DISABLE_INTS               @ Mask interrupts
    MSR     CPSR_cxsf, r1                       @ Setup new CPSR
#ifdef __THUMB_INTERWORK
    BX      lr                                  @ Return to caller
#else
    MOV     pc, lr                              @ Return to caller
#endif
@}


