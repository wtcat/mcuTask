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
#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif
@
    .global  _tx_thread_system_state
    .global  _tx_thread_current_ptr
    .global  _tx_execution_isr_enter
@
@
    .text 32
    .align 4
    .syntax unified
@/**************************************************************************/
@/*                                                                        */
@/*  FUNCTION                                               RELEASE        */
@/*                                                                        */
@/*    _tx_thread_context_save                           Cortex-M0/GNU     */
@/*                                                           6.2.1        */
@/*  AUTHOR                                                                */
@/*                                                                        */
@/*    William E. Lamie, Microsoft Corporation                             */
@/*                                                                        */
@/*  DESCRIPTION                                                           */
@/*                                                                        */
@/*    This function is only needed for legacy applications and it should  */
@/*    not be called in any new development on a Cortex-M.                 */
@/*    This function saves the context of an executing thread in the       */
@/*    beginning of interrupt processing.  The function also ensures that  */
@/*    the system stack is used upon return to the calling ISR.            */
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
@/*  05-19-2020     William E. Lamie         Initial Version 6.0           */
@/*  09-30-2020     Scott Larson             Modified comment(s), and      */
@/*                                            cleaned up whitespace,      */
@/*                                            resulting in version 6.1    */
@/*  03-08-2023      Scott Larson            Include tx_user.h,            */
@/*                                            resulting in version 6.2.1  */
@/*                                                                        */
@/**************************************************************************/
@VOID   _tx_thread_context_save(VOID)
@{
    .global  _tx_thread_context_save
    .thumb_func
_tx_thread_context_save:
@
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))        
    /* Call the ISR enter function to indicate an ISR is starting.  */
    PUSH    {r0, lr}                                // Save return address
    BL      _tx_execution_isr_enter                 // Call the ISR enter function
    POP     {r0, lr}                                // Recover return address
#endif

    /* Context is already saved - just return.  */
    
    BX      lr
@}
