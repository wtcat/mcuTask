;/***************************************************************************
; * Copyright (c) 2024 Microsoft Corporation 
; * 
; * This program and the accompanying materials are made available under the
; * terms of the MIT License which is available at
; * https://opensource.org/licenses/MIT.
; * 
; * SPDX-License-Identifier: MIT
; **************************************************************************/
;
;
;/**************************************************************************/
;/**************************************************************************/
;/**                                                                       */ 
;/** ThreadX Component                                                     */ 
;/**                                                                       */
;/**   Thread                                                              */
;/**                                                                       */
;/**************************************************************************/
;/**************************************************************************/
;
;#define TX_SOURCE_CODE
;
;
;/* Include necessary system files.  */
;
;#include "tx_api.h"
;#include "tx_thread.h"
;#include "tx_timer.h"
;
#ifdef TX_ENABLE_FIQ_SUPPORT
DISABLE_INTS    DEFINE  0xC0                ; IRQ & FIQ interrupts disabled
#else
DISABLE_INTS    DEFINE  0x80                ; IRQ interrupts disabled
#endif
;
;
    EXTERN      _tx_thread_current_ptr
    EXTERN      _tx_timer_time_slice
    EXTERN      _tx_thread_schedule
    EXTERN      _tx_execution_thread_exit
;
;
;
;/**************************************************************************/ 
;/*                                                                        */ 
;/*  FUNCTION                                               RELEASE        */ 
;/*                                                                        */ 
;/*    _tx_thread_system_return                           Cortex-A5/IAR    */ 
;/*                                                           6.1.9        */
;/*  AUTHOR                                                                */
;/*                                                                        */
;/*    William E. Lamie, Microsoft Corporation                             */
;/*                                                                        */
;/*  DESCRIPTION                                                           */
;/*                                                                        */ 
;/*    This function is target processor specific.  It is used to transfer */ 
;/*    control from a thread back to the ThreadX system.  Only a           */ 
;/*    minimal context is saved since the compiler assumes temp registers  */ 
;/*    are going to get slicked by a function call anyway.                 */ 
;/*                                                                        */ 
;/*  INPUT                                                                 */ 
;/*                                                                        */ 
;/*    None                                                                */ 
;/*                                                                        */ 
;/*  OUTPUT                                                                */ 
;/*                                                                        */ 
;/*    None                                                                */ 
;/*                                                                        */ 
;/*  CALLS                                                                 */ 
;/*                                                                        */ 
;/*    _tx_thread_schedule                   Thread scheduling loop        */ 
;/*                                                                        */ 
;/*  CALLED BY                                                             */ 
;/*                                                                        */ 
;/*    ThreadX components                                                  */ 
;/*                                                                        */ 
;/*  RELEASE HISTORY                                                       */ 
;/*                                                                        */ 
;/*    DATE              NAME                      DESCRIPTION             */
;/*                                                                        */
;/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
;/*  10-15-2021     William E. Lamie         Modified comment(s), added    */
;/*                                            execution profile support,  */
;/*                                            resulting in version 6.1.9  */
;/*                                                                        */
;/**************************************************************************/
;VOID   _tx_thread_system_return(VOID)
;{
    RSEG    .text:CODE:NOROOT(2)
    PUBLIC  _tx_thread_system_return
    CODE32
_tx_thread_system_return??rA
_tx_thread_system_return
;
;    /* Save minimal context on the stack.  */
;
    STMDB   sp!, {r4-r11, lr}                   ; Save minimal context
    LDR     r5, =_tx_thread_current_ptr         ; Pickup address of current ptr
    LDR     r6, [r5, #0]                        ; Pickup current thread pointer

#ifdef __ARMVFP__
    LDR     r0, [r6, #144]                      ; Pickup the VFP enabled flag
    CMP     r0, #0                              ; Is the VFP enabled?
    BEQ     _tx_skip_solicited_vfp_save         ; No, skip VFP solicited save
    VMRS    r4, FPSCR                           ; Pickup the FPSCR
    STR     r4, [sp, #-4]!                      ; Save FPSCR
#ifdef __D32__
    VSTMDB  sp!, {D16-D31}                      ; Save D16-D31
#endif
    VSTMDB  sp!, {D8-D15}                       ; Save D8-D15
_tx_skip_solicited_vfp_save:
#endif

    MOV     r0, #0                              ; Build a solicited stack type
    MRS     r1, CPSR                            ; Pickup the CPSR
    STMDB   sp!, {r0-r1}                        ; Save type and CPSR
;   
;   /* Lockout interrupts.  */
;
    ORR     r2, r1, #DISABLE_INTS               ; Build disable interrupt CPSR
    MSR     CPSR_cxsf, r2                       ; Disable interrupts

#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
;
;    /* Call the thread exit function to indicate the thread is no longer executing.  */
;
    BL      _tx_execution_thread_exit           ; Call the thread exit function
#endif

    LDR     r2, =_tx_timer_time_slice           ; Pickup address of time slice
    LDR     r1, [r2, #0]                        ; Pickup current time slice
;
;    /* Save current stack and switch to system stack.  */
;    _tx_thread_current_ptr -> tx_thread_stack_ptr =  sp;
;    sp = _tx_thread_system_stack_ptr;
;
    STR     sp, [r6, #8]                        ; Save thread stack pointer
;
;    /* Determine if the time-slice is active.  */
;    if (_tx_timer_time_slice)
;    {
;
    MOV     r4, #0                              ; Build clear value
    CMP     r1, #0                              ; Is a time-slice active?
    BEQ     __tx_thread_dont_save_ts            ; No, don't save the time-slice
;
;       /* Save time-slice for the thread and clear the current time-slice.  */
;       _tx_thread_current_ptr -> tx_thread_time_slice =  _tx_timer_time_slice;
;       _tx_timer_time_slice =  0;
;
    STR     r4, [r2, #0]                        ; Clear time-slice
    STR     r1, [r6, #24]                       ; Save current time-slice
;
;    }
__tx_thread_dont_save_ts
;
;    /* Clear the current thread pointer.  */
;    _tx_thread_current_ptr =  TX_NULL;
;
    STR     r4, [r5, #0]                        ; Clear current thread pointer
    B       _tx_thread_schedule                 ; Jump to scheduler!
;
;}
    END

