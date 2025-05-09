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
/**   Module                                                              */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

    .global     __use_two_region_memory
    .global     __scatterload

    .eabi_attribute Tag_ABI_PCS_RO_data, 1
    .eabi_attribute Tag_ABI_PCS_R9_use,  1
    .eabi_attribute Tag_ABI_PCS_RW_data, 2

    .text
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _txm_module_initialize                            Cortex-M33/AC6    */
/*                                                           6.1.10       */
/*  AUTHOR                                                                */
/*                                                                        */
/*    Scott Larson, Microsoft Corporation                                 */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function initializes the module c runtime.                     */
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
/*    __scatterload                         Initialize C runtime          */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _txm_module_thread_shell_entry        Start module thread           */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  12-31-2020      Scott Larson            Initial Version 6.1.3         */
/*  01-31-2022      Scott Larson            Modified comments, fixed      */
/*                                            scatterload, and made       */
/*                                            heap user configurable,     */
/*                                            resulting in version 6.1.10 */
/*                                                                        */
/**************************************************************************/
    .global  _txm_module_initialize
    .thumb_func
_txm_module_initialize:
    PUSH    {r0-r12,lr}                         // Save dregs and LR
    B       __scatterload                       // Call ARM func to initialize variables

// Override the __rt_exit function.
    .global  __rt_exit
    .thumb_func
__rt_exit:
    POP     {r4-r12,lr}                         // Restore dregs and LR
    BX      lr                                  // Return to caller

// Override the __rt_entry function.
    .global __rt_entry
    .type   __rt_entry, %function
__rt_entry:
    POP     {r0-r1}
    BL      __rt_lib_init                       // Call ARM func to initialize library
    POP     {r2-r12,lr}                         // Restore dregs and LR
    BX      lr                                  // Return to caller
