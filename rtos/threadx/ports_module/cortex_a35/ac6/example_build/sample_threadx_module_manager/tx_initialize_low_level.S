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
/**   Initialize                                                          */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

	.text
	.align 3
/**************************************************************************/ 
/*                                                                        */ 
/*  FUNCTION                                               RELEASE        */ 
/*                                                                        */ 
/*    _tx_initialize_low_level                          Cortex-A35/AC6    */ 
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */ 
/*    This function is responsible for any low-level processor            */ 
/*    initialization, including setting up interrupt vectors, setting     */ 
/*    up a periodic timer interrupt source, saving the system stack       */ 
/*    pointer for use in ISR processing later, and finding the first      */ 
/*    available RAM memory address for tx_application_define.             */ 
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
/*    _tx_initialize_kernel_enter           ThreadX entry function        */ 
/*                                                                        */ 
/*  RELEASE HISTORY                                                       */ 
/*                                                                        */ 
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
/*                                                                        */
/**************************************************************************/
/* VOID   _tx_initialize_low_level(VOID)
{ */
    .global  _tx_initialize_low_level
    .type    _tx_initialize_low_level, @function
_tx_initialize_low_level:

    MSR     DAIFSet, 0x3                        // Lockout interrupts


    /* Save the system stack pointer.  */
    /* _tx_thread_system_stack_ptr = (VOID_PTR) (sp);  */

    LDR     x0, =_tx_thread_system_stack_ptr    // Pickup address of system stack ptr
    MOV     x1, sp                              // Pickup SP
    BIC     x1, x1, #0xF                        // Get 16-bit alignment
    STR     x1, [x0]                            // Store system stack

    /* Save the first available memory address.  */
    /* _tx_initialize_unused_memory =  (VOID_PTR) Image$$ZI$$Limit;  */

    LDR     x0, =_tx_initialize_unused_memory   // Pickup address of unused memory ptr
    LDR     x1, =zi_limit                       // Pickup unused memory address
    LDR     x1, [x1]                            //
    STR     x1, [x0]                            // Store unused memory address

    /* Done, return to caller.  */

    RET                                         // Return to caller
/* } */


zi_limit:
    .quad (Image$$TOP_OF_RAM$$Base)

