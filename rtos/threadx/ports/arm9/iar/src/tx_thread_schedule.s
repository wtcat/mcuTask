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
ENABLE_INTS     DEFINE  0xC0                    ; IRQ & FIQ Interrupts enabled mask
#else
ENABLE_INTS     DEFINE  0x80                    ; IRQ Interrupts enabled mask
#endif
;
;
    EXTERN     _tx_thread_execute_ptr
    EXTERN     _tx_thread_current_ptr
    EXTERN     _tx_timer_time_slice
    EXTERN     _tx_execution_thread_enter
;
;
;
;/**************************************************************************/ 
;/*                                                                        */ 
;/*  FUNCTION                                               RELEASE        */ 
;/*                                                                        */ 
;/*    _tx_thread_schedule                                  ARM9/IAR       */ 
;/*                                                           6.1          */
;/*  AUTHOR                                                                */
;/*                                                                        */
;/*    William E. Lamie, Microsoft Corporation                             */
;/*                                                                        */
;/*  DESCRIPTION                                                           */
;/*                                                                        */ 
;/*    This function waits for a thread control block pointer to appear in */ 
;/*    the _tx_thread_execute_ptr variable.  Once a thread pointer appears */ 
;/*    in the variable, the corresponding thread is resumed.               */ 
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
;/*    None                                                                */
;/*                                                                        */ 
;/*  CALLED BY                                                             */ 
;/*                                                                        */ 
;/*    _tx_initialize_kernel_enter          ThreadX entry function         */ 
;/*    _tx_thread_system_return             Return to system from thread   */ 
;/*    _tx_thread_context_restore           Restore thread's context       */ 
;/*                                                                        */ 
;/*  RELEASE HISTORY                                                       */ 
;/*                                                                        */ 
;/*    DATE              NAME                      DESCRIPTION             */
;/*                                                                        */
;/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
;/*                                                                        */
;/**************************************************************************/
;VOID   _tx_thread_schedule(VOID)
;{
    RSEG    .text:CODE:NOROOT(2)
    PUBLIC  _tx_thread_schedule
    CODE32
_tx_thread_schedule??rA
_tx_thread_schedule
;
;    /* Enable interrupts.  */
;
    MRS     r2, CPSR                            ; Pickup CPSR
    BIC     r0, r2, #ENABLE_INTS                ; Clear the disable bit(s)
    MSR     CPSR_cxsf, r0                       ; Enable interrupts
;
;    /* Wait for a thread to execute.  */
;    do
;    {
    LDR     r1, =_tx_thread_execute_ptr         ; Address of thread execute ptr
;
__tx_thread_schedule_loop
;
    LDR     r0, [r1, #0]                        ; Pickup next thread to execute
    CMP     r0, #0                              ; Is it NULL?
    BEQ     __tx_thread_schedule_loop           ; If so, keep looking for a thread
;
;    }
;    while(_tx_thread_execute_ptr == TX_NULL);
;    
;    /* Yes! We have a thread to execute.  Lockout interrupts and
;       transfer control to it.  */
;
    MSR     CPSR_cxsf, r2                       ; Disable interrupts
;
;    /* Setup the current thread pointer.  */
;    _tx_thread_current_ptr =  _tx_thread_execute_ptr;
;
    LDR     r1, =_tx_thread_current_ptr         ; Pickup address of current thread 
    STR     r0, [r1, #0]                        ; Setup current thread pointer
;
;    /* Increment the run count for this thread.  */
;    _tx_thread_current_ptr -> tx_thread_run_count++;
;
    LDR     r2, [r0, #4]                        ; Pickup run counter
    LDR     r3, [r0, #24]                       ; Pickup time-slice for this thread
    ADD     r2, r2, #1                          ; Increment thread run-counter
    STR     r2, [r0, #4]                        ; Store the new run counter
;
;    /* Setup time-slice, if present.  */
;    _tx_timer_time_slice =  _tx_thread_current_ptr -> tx_thread_time_slice;
;
    LDR     r2, =_tx_timer_time_slice           ; Pickup address of time slice 
                                                ;   variable
    LDR     sp, [r0, #8]                        ; Switch stack pointers
    STR     r3, [r2, #0]                        ; Setup time-slice
;
;    /* Switch to the thread's stack.  */
;    sp =  _tx_thread_execute_ptr -> tx_thread_stack_ptr;
;
#ifdef TX_ENABLE_EXECUTION_CHANGE_NOTIFY
;
;    /* Call the thread entry function to indicate the thread is executing.  */
;
    BL      _tx_execution_thread_enter          ; Call the thread execution enter function
#endif
;
;    /* Determine if an interrupt frame or a synchronous task suspension frame
;   is present.  */
;
    LDMIA   sp!, {r0, r1}                       ; Pickup the stack type and saved CPSR
    CMP     r0, #0                              ; Check for synchronous context switch
    MSRNE   SPSR_cxsf, r1                       ;   Setup SPSR for return
    LDMNEIA sp!, {r0-r12, lr, pc}^              ; Return to point of thread interrupt
    LDMIA   sp!, {r4-r11, lr}                   ; Return to thread synchronously
    MSR     CPSR_cxsf, r1                       ;   Recover CPSR
#ifdef TX_THUMB
    BX      lr                                  ; Return to caller
#else
    MOV     pc, lr                              ; Return to caller
#endif
;
;}
;
    END

