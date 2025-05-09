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

    extern __tx_thread_system_state
    extern __tx_thread_current_ptr

    section .text:CODE:ROOT
;/**************************************************************************/
;/*                                                                        */
;/*  FUNCTION                                               RELEASE        */
;/*                                                                        */
;/*    _tx_thread_context_save                              RXv1/IAR       */
;/*                                                           6.1.11       */
;/*  AUTHOR                                                                */
;/*                                                                        */
;/*    William E. Lamie, Microsoft Corporation                             */
;/*                                                                        */
;/*  DESCRIPTION                                                           */
;/*                                                                        */
;/*    This function saves the context of an executing thread in the       */
;/*    beginning of interrupt processing.  The function also ensures that  */
;/*    the system stack is used upon return to the calling ISR.            */
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
;/*    ISRs                                                                */
;/*                                                                        */
;/*  RELEASE HISTORY                                                       */
;/*                                                                        */
;/*    DATE              NAME                      DESCRIPTION             */
;/*                                                                        */
;/*  08-02-2021     William E. Lamie         Initial Version 6.1.8         */
;/*  10-15-2021     William E. Lamie         Modified comment(s),          */
;/*                                            resulting in version 6.1.9  */
;/*  01-31-2022     William E. Lamie         Modified comment(s),          */
;/*                                            resulting in version 6.1.10 */
;/*  04-25-2022     William E. Lamie         Modified comment(s),          */
;/*                                            resulting in version 6.1.11 */
;/*                                                                        */
;/**************************************************************************/
;VOID   _tx_thread_context_save(VOID)
;{
    public __tx_thread_context_save

__tx_thread_context_save:
;
;    /* Upon entry to this routine, it is assumed that interrupts are locked
;       out and the (interrupt) stack frame looks like the following:
;
;           (lower address) SP   ->     [return address of this call]
;                           SP+4 ->     Saved R1
;                           SP+8 ->     Saved R2
;                           SP+12->     Interrupted PC
;                           SP+16->     Interrupted PSW
;
;    /* Check for a nested interrupt condition.  */
;    if (_tx_thread_system_state++)
;    {
;

    MOV.L   #__tx_thread_system_state, R1        ; Pick up address of system state
    MOV.L   [R1], R2                             ; Pick up system state
    CMP     #0, R2                               ; 0 -> no nesting
    BEQ     __tx_thread_not_nested_save
;
;    /* Nested interrupt condition.  */
;   
    ADD   #1, r2                                 ; _tx_thread_system_state++
    MOV.L   r2, [r1]

;
;   /* Save the rest of the scratch registers on the interrupt stack and return to the
;       calling ISR.  */
    POP R1                                       ; Recuperate return address from stack
    PUSHM   R3-R5
    PUSHM   R14-R15
    JMP     R1                                   ; Return address was preserved in R1

;
__tx_thread_not_nested_save:
;    }
;
;    /* Otherwise, not nested, check to see if a thread was running.  */
;    else if (_tx_thread_current_ptr)
;    {
;
    ADD     #1, R2                               ; _tx_thread_system_state++
    MOV.L   R2, [R1]

    MOV.L   #__tx_thread_current_ptr, R2         ; Pickup current thread pointer
    MOV.L   [R2], R2
    CMP     #0,R2                                ; Is it NULL?  
    BEQ      __tx_thread_idle_system_save        ; Yes, idle system is running - idle restore
;
;    /* Move stack frame over to the current threads stack.  */
;    /* complete stack frame with registers not saved yet (R3-R5, R14-R15, FPSW)   */
;
    MVFC    USP, R1                              ; Pick up user stack pointer
    MOV.L   16[R0], R2
    MOV.L   R2, [-R1]                            ; Save PSW on thread stack
    MOV.L   12[R0], R2
    MOV.L   R2, [-R1]                            ; Save PC on thread stack
    MOV.L   8[R0], R2
    MOV.L   R2, [-R1]                            ; Save R2 on thread stack
    MOV.L   4[R0], R2
    MOV.L   R2, [-R1]                            ; Save R1 on thread stack
    MOV.L   R5, [-R1]                            ; Save R5 on thread stack
    MOV.L   R4, [-R1]                            ; Save R4 on thread stack
    MOV.L   R3, [-R1]                            ; Save R3 on thread stack
    MOV.L   R15, [-R1]                           ; Save R15 on thread stack
    MOV.L   R14, [-R1]                           ; Save R14 on thread stack
	
    POP     R2                                   ; Pick up return address from interrupt stack
    ADD     #16, R0, R0                          ; Correct interrupt stack pointer back to the bottom
    MVTC    R1, USP                              ; Set user/thread stack pointer
    JMP     R2                                   ; Return to ISR

;    }
;    else
;    {
;
__tx_thread_idle_system_save:
;
;        /* Interrupt occurred in the scheduling loop.  */
;
    POP     R1                                   ; Pick up return address
    ADD     #16, R0, R0                          ; Correct interrupt stack pointer back to the bottom (PC), don't care about saved registers
    JMP     R1                                   ; Return to caller
;
;    }
;}    
    END
