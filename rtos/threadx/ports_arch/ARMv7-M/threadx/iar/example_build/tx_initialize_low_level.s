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

    EXTERN  _tx_thread_system_stack_ptr
    EXTERN  _tx_initialize_unused_memory
    EXTERN  _tx_timer_interrupt
    EXTERN  __vector_table
    EXTERN  _tx_execution_isr_enter
    EXTERN  _tx_execution_isr_exit

SYSTEM_CLOCK      EQU   25000000
SYSTICK_CYCLES    EQU   ((SYSTEM_CLOCK / 100) -1)

    RSEG    FREE_MEM:DATA
    PUBLIC  __tx_free_memory_start
__tx_free_memory_start
    DS32    4

    SECTION `.text`:CODE:NOROOT(2)
    THUMB
/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _tx_initialize_low_level                          Cortex-Mx/IAR     */
/*                                                           6.1.10       */
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
/*  06-02-2021      Scott Larson            Initial Version 6.1.7         */
/*  01-31-2022      Scott Larson            Modified comment(s), added    */
/*                                            TX_NO_TIMER support,        */
/*                                            resulting in version 6.1.10 */
/*                                                                        */
/**************************************************************************/
// VOID   _tx_initialize_low_level(VOID)
// {
    PUBLIC  _tx_initialize_low_level
_tx_initialize_low_level:

    /* Ensure that interrupts are disabled.  */
    CPSID   i

    /* Set base of available memory to end of non-initialised RAM area.  */
    LDR     r0, =__tx_free_memory_start             // Get end of non-initialized RAM area
    LDR     r2, =_tx_initialize_unused_memory       // Build address of unused memory pointer
    STR     r0, [r2, #0]                            // Save first free memory address

    /* Setup Vector Table Offset Register.  */
    MOV     r0, #0xE000E000                         // Build address of NVIC registers
    LDR     r1, =__vector_table                     // Pickup address of vector table
    STR     r1, [r0, #0xD08]                        // Set vector table address

    /* Set system stack pointer from vector value.  */
    LDR     r0, =_tx_thread_system_stack_ptr        // Build address of system stack pointer
    LDR     r1, =__vector_table                     // Pickup address of vector table
    LDR     r1, [r1]                                // Pickup reset stack pointer
    STR     r1, [r0]                                // Save system stack pointer

#ifndef TX_NO_TIMER
    /* Configure SysTick.  */
    MOV     r0, #0xE000E000                         // Build address of NVIC registers
    LDR     r1, =SYSTICK_CYCLES
    STR     r1, [r0, #0x14]                         // Setup SysTick Reload Value
    MOV     r1, #0x7                                // Build SysTick Control Enable Value
    STR     r1, [r0, #0x10]                         // Setup SysTick Control
#endif

    /* Configure handler priorities.  */
    LDR     r1, =0x00000000                         // Rsrv, UsgF, BusF, MemM
    STR     r1, [r0, #0xD18]                        // Setup System Handlers 4-7 Priority Registers
    LDR     r1, =0xFF000000                         // SVCl, Rsrv, Rsrv, Rsrv
    STR     r1, [r0, #0xD1C]                        // Setup System Handlers 8-11 Priority Registers
                                                    // Note: SVC must be lowest priority, which is 0xFF
    LDR     r1, =0x40FF0000                         // SysT, PnSV, Rsrv, DbgM
    STR     r1, [r0, #0xD20]                        // Setup System Handlers 12-15 Priority Registers
                                                    // Note: PnSV must be lowest priority, which is 0xFF

    /* Return to caller.  */
    BX      lr
// }

#ifndef TX_NO_TIMER
    PUBLIC  SysTick_Handler
    PUBLIC  __tx_SysTickHandler
__tx_SysTickHandler:
SysTick_Handler:
// VOID SysTick_Handler(VOID)
// {

    PUSH    {r0, lr}
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
    BL      _tx_execution_isr_enter             // Call the ISR enter function
#endif
    BL      _tx_timer_interrupt
#if (defined(TX_ENABLE_EXECUTION_CHANGE_NOTIFY) || defined(TX_EXECUTION_PROFILE_ENABLE))
    BL      _tx_execution_isr_exit              // Call the ISR exit function
#endif
    POP     {r0, lr}
    BX      lr
// }
#endif
    END
