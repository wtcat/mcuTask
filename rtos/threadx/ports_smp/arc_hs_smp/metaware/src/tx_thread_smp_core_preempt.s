;/***************************************************************************
; * Copyright (c) 2024 Microsoft Corporation 
; * 
; * This program and the accompanying materials are made available under the
; * terms of the MIT License which is available at
; * https://opensource.org/licenses/MIT.
; * 
; * SPDX-License-Identifier: MIT
; **************************************************************************/
;
;
;/**************************************************************************/
;/**************************************************************************/
;/**                                                                       */ 
;/** ThreadX Component                                                     */ 
;/**                                                                       */
;/**   Thread - Low Level SMP Support                                      */
;/**                                                                       */
;/**************************************************************************/
;/**************************************************************************/
;
;
;#define TX_SOURCE_CODE
;#define TX_THREAD_SMP_SOURCE_CODE
;
;/* Include necessary system files.  */
;
;#include "tx_api.h"
;#include "tx_thread.h"
;#include "tx_timer.h"  */
;
;
;/**************************************************************************/ 
;/*                                                                        */ 
;/*  FUNCTION                                               RELEASE        */ 
;/*                                                                        */ 
;/*    _tx_thread_smp_core_preempt                     SMP/ARC_HS/MetaWare */ 
;/*                                                            6.1         */
;/*  AUTHOR                                                                */
;/*                                                                        */
;/*    William E. Lamie, Microsoft Corporation                             */
;/*                                                                        */
;/*  DESCRIPTION                                                           */
;/*                                                                        */ 
;/*    This function preempts the specified core in situations where the   */ 
;/*    thread corresponding to this core is no longer ready or when the    */ 
;/*    core must be used for a higher-priority thread. If the specified is */ 
;/*    the current core, this processing is skipped since the will give up */ 
;/*    control subsequently on its own.                                    */ 
;/*                                                                        */ 
;/*  INPUT                                                                 */ 
;/*                                                                        */ 
;/*    core                                  The core to preempt           */ 
;/*                                                                        */ 
;/*  OUTPUT                                                                */ 
;/*                                                                        */ 
;/*    None                                                                */
;/*                                                                        */ 
;/*  CALLS                                                                 */ 
;/*                                                                        */ 
;/*    None                                                                */
;/*                                                                        */ 
;/*  CALLED BY                                                             */ 
;/*                                                                        */ 
;/*    ThreadX Source                                                      */
;/*                                                                        */ 
;/*  RELEASE HISTORY                                                       */ 
;/*                                                                        */ 
;/*    DATE              NAME                      DESCRIPTION             */
;/*                                                                        */
;/*  09-30-2020     William E. Lamie         Initial Version 6.1           */
;/*                                                                        */
;/**************************************************************************/
    .global _tx_thread_smp_core_preempt
    .type   _tx_thread_smp_core_preempt, @function
_tx_thread_smp_core_preempt:
    sub     sp, sp, 16                          ; Allocate some stack space
    st      blink, [sp]                         ; Save return address
    bl.d    arc_ici_send                        ; Call ARC inter-core interrupt routine
    sub     sp, sp, 16                          ; Allocate stack space (delay slot)
    add     sp, sp, 16                          ; Recover stack space
    ld      blink, [sp]                         ; Recover return address  
    j_s.d   [blink]                             ; Return to caller with delay slot
    add     sp, sp, 16                          ; Recover stack space

    .end
    
