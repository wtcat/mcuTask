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
@
@
@#define TX_SOURCE_CODE
@
@
@/* Include necessary system files.  */
@
@#include "tx_api.h"
@#include "tx_thread.h"
@#include "tx_timer.h"
@
@
SVC_MODE        =       0xD3                    @ SVC mode
FIQ_MODE        =       0xD1                    @ FIQ mode
MODE_MASK       =       0x1F                    @ Mode mask 
THUMB_MASK      =       0x20                    @ Thumb bit mask
IRQ_MODE_BITS   =       0x12                    @ IRQ mode bits
@
@
    .global     _tx_thread_system_state
    .global     _tx_thread_current_ptr
    .global     _tx_thread_system_stack_ptr
    .global     _tx_thread_execute_ptr
    .global     _tx_timer_time_slice
    .global     _tx_thread_schedule
    .global     _tx_thread_preempt_disable
@
@
@/* No 16-bit Thumb mode veneer code is needed for _tx_thread_fiq_context_restore
@   since it will never be called 16-bit mode.  */
@
    .arm
    .text
    .align 2
@/**************************************************************************/ 
@/*                                                                        */ 
@/*  FUNCTION                                               RELEASE        */ 
@/*                                                                        */ 
@/*    _tx_thread_fiq_context_restore                    Cortex-R5/AC6     */ 
@/*                                                            6.1         */
@/*  AUTHOR                                                                */
@/*                                                                        */
@/*    William E. Lamie, Microsoft Corporation                             */
@/*                                                                        */
@/*  DESCRIPTION                                                           */
@/*                                                                        */ 
@/*    This function restores the fiq interrupt context when processing a  */ 
@/*    nested interrupt.  If not, it returns to the interrupt thread if no */ 
@/*    preemption is necessary.  Otherwise, if preemption is necessary or  */ 
@/*    if no thread was running, the function returns to the scheduler.    */ 
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
@/*    _tx_thread_schedule                   Thread scheduling routine     */ 
@/*                                                                        */ 
@/*  CALLED BY                                                             */ 
@/*                                                                        */ 
@/*    FIQ ISR                               Interrupt Service Routines    */ 
@/*                                                                        */ 
@/*  RELEASE HISTORY                                                       */ 
@/*                                                                        */ 
@/*    DATE              NAME                      DESCRIPTION             */
@/*                                                                        */
@/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
@/*                                                                        */
@/**************************************************************************/
@VOID   _tx_thread_fiq_context_restore(VOID)
@{
    .global  _tx_thread_fiq_context_restore
    .type    _tx_thread_fiq_context_restore,function
_tx_thread_fiq_context_restore:
@
@    /* Lockout interrupts.  */
@
    CPSID   if                                  @ Disable IRQ and FIQ interrupts

#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
@
@    /* Call the ISR exit function to indicate an ISR is complete.  */
@
    BL      _tx_execution_isr_exit              @ Call the ISR exit function
#endif
@
@    /* Determine if interrupts are nested.  */
@    if (--_tx_thread_system_state)
@    {
@
    LDR     r3, =_tx_thread_system_state        @ Pickup address of system state variable
    LDR     r2, [r3]                            @ Pickup system state
    SUB     r2, r2, #1                          @ Decrement the counter
    STR     r2, [r3]                            @ Store the counter 
    CMP     r2, #0                              @ Was this the first interrupt?
    BEQ     __tx_thread_fiq_not_nested_restore  @ If so, not a nested restore
@
@    /* Interrupts are nested.  */
@
@    /* Just recover the saved registers and return to the point of 
@       interrupt.  */
@
    LDMIA   sp!, {r0, r10, r12, lr}             @ Recover SPSR, POI, and scratch regs
    MSR     SPSR_cxsf, r0                       @ Put SPSR back
    LDMIA   sp!, {r0-r3}                        @ Recover r0-r3
    MOVS    pc, lr                              @ Return to point of interrupt
@
@    }
__tx_thread_fiq_not_nested_restore:
@
@    /* Determine if a thread was interrupted and no preemption is required.  */
@    else if (((_tx_thread_current_ptr) && (_tx_thread_current_ptr == _tx_thread_execute_ptr))
@               || (_tx_thread_preempt_disable))
@    {
@
    LDR     r1, [sp]                            @ Pickup the saved SPSR
    MOV     r2, #MODE_MASK                      @ Build mask to isolate the interrupted mode
    AND     r1, r1, r2                          @ Isolate mode bits
    CMP     r1, #IRQ_MODE_BITS                  @ Was an interrupt taken in IRQ mode before we
                                                @   got to context save? */
    BEQ     __tx_thread_fiq_no_preempt_restore  @ Yes, just go back to point of interrupt


    LDR     r1, =_tx_thread_current_ptr         @ Pickup address of current thread ptr
    LDR     r0, [r1]                            @ Pickup actual current thread pointer
    CMP     r0, #0                              @ Is it NULL?
    BEQ     __tx_thread_fiq_idle_system_restore @ Yes, idle system was interrupted

    LDR     r3, =_tx_thread_preempt_disable     @ Pickup preempt disable address
    LDR     r2, [r3]                            @ Pickup actual preempt disable flag
    CMP     r2, #0                              @ Is it set?
    BNE     __tx_thread_fiq_no_preempt_restore  @ Yes, don't preempt this thread
    LDR     r3, =_tx_thread_execute_ptr         @ Pickup address of execute thread ptr
    LDR     r2, [r3]                            @ Pickup actual execute thread pointer
    CMP     r0, r2                              @ Is the same thread highest priority?
    BNE     __tx_thread_fiq_preempt_restore     @ No, preemption needs to happen


__tx_thread_fiq_no_preempt_restore:
@
@    /* Restore interrupted thread or ISR.  */
@
@    /* Pickup the saved stack pointer.  */
@    tmp_ptr =  _tx_thread_current_ptr -> tx_thread_stack_ptr;
@
@    /* Recover the saved context and return to the point of interrupt.  */
@
    LDMIA   sp!, {r0, lr}                       @ Recover SPSR, POI, and scratch regs
    MSR     SPSR_cxsf, r0                       @ Put SPSR back
    LDMIA   sp!, {r0-r3}                        @ Recover r0-r3
    MOVS    pc, lr                              @ Return to point of interrupt
@
@    }
@    else
@    {
__tx_thread_fiq_preempt_restore:
@
    LDMIA   sp!, {r3, lr}                       @ Recover temporarily saved registers
    MOV     r1, lr                              @ Save lr (point of interrupt)
    MOV     r2, #SVC_MODE                       @ Build SVC mode CPSR
    MSR     CPSR_c, r2                          @ Enter SVC mode
    STR     r1, [sp, #-4]!                      @ Save point of interrupt
    STMDB   sp!, {r4-r12, lr}                   @ Save upper half of registers
    MOV     r4, r3                              @ Save SPSR in r4
    MOV     r2, #FIQ_MODE                       @ Build FIQ mode CPSR
    MSR     CPSR_c, r2                          @ Reenter FIQ mode
    LDMIA   sp!, {r0-r3}                        @ Recover r0-r3
    MOV     r5, #SVC_MODE                       @ Build SVC mode CPSR
    MSR     CPSR_c, r5                          @ Enter SVC mode
    STMDB   sp!, {r0-r3}                        @ Save r0-r3 on thread's stack
    MOV     r3, #1                              @ Build interrupt stack type
    STMDB   sp!, {r3, r4}                       @ Save interrupt stack type and SPSR
    LDR     r1, =_tx_thread_current_ptr         @ Pickup address of current thread ptr
    LDR     r0, [r1]                            @ Pickup current thread pointer
    STR     sp, [r0, #8]                        @ Save stack pointer in thread control
                                                @   block  */
@
@    /* Save the remaining time-slice and disable it.  */
@    if (_tx_timer_time_slice)
@    {
@
    LDR     r3, =_tx_timer_time_slice           @ Pickup time-slice variable address
    LDR     r2, [r3]                            @ Pickup time-slice
    CMP     r2, #0                              @ Is it active?
    BEQ     __tx_thread_fiq_dont_save_ts        @ No, don't save it
@
@        _tx_thread_current_ptr -> tx_thread_time_slice =  _tx_timer_time_slice;
@        _tx_timer_time_slice =  0; 
@
    STR     r2, [r0, #24]                       @ Save thread's time-slice
    MOV     r2, #0                              @ Clear value
    STR     r2, [r3]                            @ Disable global time-slice flag
@
@    }
__tx_thread_fiq_dont_save_ts:
@
@
@    /* Clear the current task pointer.  */
@    _tx_thread_current_ptr =  TX_NULL;
@
    MOV     r0, #0                              @ NULL value
    STR     r0, [r1]                            @ Clear current thread pointer
@
@    /* Return to the scheduler.  */
@    _tx_thread_schedule();
@
    B       _tx_thread_schedule                 @ Return to scheduler
@    }
@
__tx_thread_fiq_idle_system_restore:
@
@    /* Just return back to the scheduler!  */
@
    ADD     sp, sp, #24                         @ Recover FIQ stack space
    MOV     r3, #SVC_MODE                       @ Build SVC mode CPSR
    MSR     CPSR_c, r3                          @ Lockout interrupts
    B       _tx_thread_schedule                 @ Return to scheduler
@
@}

