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

#ifdef TX_INCLUDE_USER_DEFINE_FILE
#include "tx_user.h"
#endif

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_secure_stack_allocate                  Cortex-M23/GNU    */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function enters the SVC handler to allocate a secure stack.    */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    thread_ptr                            Thread control block pointer  */
/*    stack_size                            Size of secure stack to       */
/*                                            allocate                    */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    status                                Actual completion status      */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    SVC 1                                                               */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020      Scott Larson            Initial Version 6.1           */
/*  03-08-2023      Scott Larson            Include tx_user.h,            */
/*                                            resulting in version 6.2.1  */
/*                                                                        */
/**************************************************************************/
// UINT   _tx_thread_secure_stack_allocate(TX_THREAD *thread_ptr, ULONG stack_size)
// {
    .section .text
    .balign 4
    .syntax unified
    .eabi_attribute Tag_ABI_align_preserved, 1
    .global  _tx_thread_secure_stack_allocate
    .thumb_func
.type _tx_thread_secure_stack_allocate, function
_tx_thread_secure_stack_allocate:
#if !defined(TX_SINGLE_MODE_SECURE) && !defined(TX_SINGLE_MODE_NON_SECURE)
    MRS     r3, PRIMASK     // Save interrupt mask
    CPSIE   i               // Enable interrupts for SVC call
    SVC     1
    CMP     r3, #0          // If interrupts enabled, just return
    BEQ     _alloc_return_interrupt_enabled
    CPSID   i               // Otherwise, disable interrupts
#else
    MOVS    r0, #0xFF       // Feature not enabled
#endif
_alloc_return_interrupt_enabled:
    BX      lr
    .end
