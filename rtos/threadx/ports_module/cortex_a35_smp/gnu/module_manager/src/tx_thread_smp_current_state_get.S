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
/**   Thread - Low Level SMP Support                                      */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/


    .text
    .align 3
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_smp_current_state_get                 Cortex-A35-SMP/GNU */
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is gets the current state of the calling core.        */
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
/*  12-31-2020     William E. Lamie         Initial Version 6.1.3         */
/*                                                                        */
/**************************************************************************/
    .global  _tx_thread_smp_current_state_get
    .type    _tx_thread_smp_current_state_get, @function
_tx_thread_smp_current_state_get:

    MRS     x1, DAIF                            // Pickup current interrupt posture
    MSR     DAIFSet, 0x3                        // Lockout interrupts
    MRS     x2, MPIDR_EL1                       // Pickup the core ID
#if TX_THREAD_SMP_CLUSTERS > 1
    UBFX    x3, x2, #8, #8                      // Isolate cluster ID
#endif
    UBFX    x2, x2, #0, #8                      // Isolate core ID
#if TX_THREAD_SMP_CLUSTERS > 1
    ADDS    x2, x2, x3, LSL #2                  // Calculate CPU ID
#endif
    LDR     x3, =_tx_thread_system_state        // Pickup the base of the current system state array
    LDR     w0, [x3, x2, LSL #2]                // Pickup the current system state for this core
    MSR     DAIF, x1                            // Restore interrupt posture
    RET
