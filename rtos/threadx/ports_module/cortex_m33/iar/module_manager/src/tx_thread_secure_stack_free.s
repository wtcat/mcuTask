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

    SECTION `.text`:CODE:NOROOT(2)
    THUMB
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_secure_stack_free                      Cortex-M33/IAR    */
/*                                                           6.1.5        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function enters the SVC handler to free a secure stack.        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    thread_ptr                            Thread control block pointer  */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    status                                Actual completion status      */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    SVC 2                                                               */
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
/*  03-02-2021      Scott Larson            Modified comment(s),          */
/*                                            resulting in version 6.1.5  */
/*                                                                        */
/**************************************************************************/
// UINT   _tx_thread_secure_stack_free(TX_THREAD *thread_ptr)
// {
    EXPORT  _tx_thread_secure_stack_free
_tx_thread_secure_stack_free:
#if !defined(TX_SINGLE_MODE_SECURE) && !defined(TX_SINGLE_MODE_NON_SECURE)
    MRS     r3, PRIMASK     // Save interrupt mask
    CPSIE   i               // Enable interrupts for SVC call
    SVC     2
    EXPORT _tx_free_return
_tx_free_return:
    CMP     r3, #0          // If interrupts enabled, just return
    BEQ     _free_return_interrupt_enabled
    CPSID   i               // Otherwise, disable interrupts
#else
    MOV     r0, #0xFF       // Feature not enabled
#endif
_free_return_interrupt_enabled
    BX      lr
    END
