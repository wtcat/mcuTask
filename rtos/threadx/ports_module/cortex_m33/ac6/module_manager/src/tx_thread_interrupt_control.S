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


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_interrupt_control                      Cortex-M33/AC6    */
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is responsible for changing the interrupt lockout     */
/*    posture of the system.                                              */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    new_posture                           New interrupt lockout posture */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    old_posture                           Old interrupt lockout posture */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  12-31-2020      Scott Larson            Initial Version 6.1.3         */        
/*                                                                        */
/**************************************************************************/
// UINT   _tx_thread_interrupt_control(UINT new_posture)
// {
    .section .text
    .balign 4
    .eabi_attribute Tag_ABI_align_preserved, 1
    .global  _tx_thread_interrupt_control
    .thumb_func
.type _tx_thread_interrupt_control, function
_tx_thread_interrupt_control:
#ifdef TX_PORT_USE_BASEPRI
    MRS     r1, BASEPRI                         // Pickup current interrupt posture
    MSR     BASEPRI, r0                         // Apply the new interrupt posture
    MOV     r0, r1                              // Transfer old to return register
#else
    MRS     r1, PRIMASK                         // Pickup current interrupt lockout
    MSR     PRIMASK, r0                         // Apply the new interrupt lockout
    MOV     r0, r1                              // Transfer old to return register
#endif
    BX      lr                                  // Return to caller
// }
    .end
