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


    .text
    .align 3
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_thread_interrupt_restore                     Cortex-A35-SMP/GNU */
/*                                                           6.1.3        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Andres Mlinar, Microsoft Corporation                                */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function is responsible for restoring interrupts to the state  */
/*    returned by a previous _tx_thread_interrupt_disable call.           */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    old_posture                           Old interrupt lockout posture */
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
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  12-31-2020     Andres Mlinar            Initial Version 6.1.3         */
/*                                                                        */
/**************************************************************************/
// UINT   _tx_thread_interrupt_restore(UINT old_posture)
// {
    .global _tx_thread_interrupt_restore
    .type   _tx_thread_interrupt_restore, @function
_tx_thread_interrupt_restore:

    /* Restore the old interrupt posture.  */

    MSR     DAIF, x0                            // Setup the old posture
    RET                                         // Return to caller

// }
