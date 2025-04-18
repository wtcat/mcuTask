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

#ifndef TXM_MODULE_HEAP_SIZE
#define TXM_MODULE_HEAP_SIZE 512
#endif

    .text

// module entry point
    .align 3
    .global _txm_module_initialize
_txm_module_initialize:
    sub     sp, sp, 16
    str     x30, [sp]
    B       __main

// main function called by the C library
    .align 3
    .global main
main:
    // Do nothing
    RET

// custom exit, return to the initialization caller
    .align 3
    .global exit
exit:
    ldr     x30, [sp]
    add     sp, sp, 16
    RET

// returns heap start address in  R0
// returns heap end address in    R2
// does not touch SP, it is already set up before the module runs
    .align 3
    .global __user_setup_stackheap
__user_setup_stackheap:
    LDR     x0, =_txm_heap                      // load heap offset
    MOV     x2, TXM_MODULE_HEAP_SIZE            // load heap size
    ADD     x2, x2, x0                          // calculate heap end address
    RET

    .align 8
_txm_heap:
    .zero   TXM_MODULE_HEAP_SIZE
