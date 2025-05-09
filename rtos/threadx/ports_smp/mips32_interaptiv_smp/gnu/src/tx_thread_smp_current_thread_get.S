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


#define UserLocal       $4,2
#define C0_Status       $12

    .text
    .set        noreorder
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_smp_current_thread_get             MIPS32_interAptiv/GNU */
/*                                                           6.2.1        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is gets the current thread of the calling core.       */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    ThreadX Components                                                  */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  03-08-2023      Scott Larson            Initial Version 6.2.1         */
/*                                                                        */
/**************************************************************************/
    .globl  _tx_thread_smp_current_thread_get
_tx_thread_smp_current_thread_get:
    di      $10                                 # Disable interrupts
    ehb                                         #
    mfc0    $25, UserLocal                      # Pickup VPE ID
    la      $12, _tx_thread_current_ptr         # Pickup the current thread pointer
    sll     $25, $25, 2                         # Build index based on VPE number
    addu    $12, $12, $25                       # Build address of current thread pointer for this VPE
    lw      $2, ($12)                           # Pickup current thread pointer
    mtc0    $10, C0_Status                      # Restore interrupt posture
    jr.hb   $31                                 # Return to caller
    nop                                         # Delay slot
