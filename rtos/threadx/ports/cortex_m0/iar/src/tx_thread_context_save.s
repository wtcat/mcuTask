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
    EXTERN  _tx_thread_system_state
    EXTERN  _tx_thread_current_ptr
    EXTERN  _tx_execution_isr_enter
;
;
    SECTION `.text`:CODE:NOROOT(2)
    THUMB
;/**************************************************************************/
;/*                                                                        */
;/*  FUNCTION                                               RELEASE        */
;/*                                                                        */
;/*    _tx_thread_context_save                           Cortex-M0/IAR     */
;/*                                                           6.1          */
;/*  AUTHOR                                                                */
;/*                                                                        */
;/*    William E. Lamie, Microsoft Corporation                             */
;/*                                                                        */
;/*  DESCRIPTION                                                           */
;/*                                                                        */
;/*    This function is only needed for legacy applications and it should  */
;/*    not be called in any new development on a Cortex-M.                 */
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
;/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
;/*                                                                        */
;/**************************************************************************/
;VOID   _tx_thread_context_save(VOID)
;{
    PUBLIC  _tx_thread_context_save
_tx_thread_context_save:
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE)) 
;
;    /* Call the ISR enter function to indicate an ISR is starting.  */
;
    PUSH    {r0, lr}                                ; Save return address
    BL      _tx_execution_isr_enter                 ; Call the ISR enter function
    POP     {r0, r1}                                ; Recover return address
    MOV     lr, r1                                  ;
#endif
;
;    /* Context is already saved - just return!  */
;
    BX      lr
;}
    END
