/*
 * Copyright (c) 1997-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SYS_ATOMIC_C_H_
#define SYS_ATOMIC_C_H_

#include <tx_api.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Simple and correct (but very slow) implementation of atomic
 * primitives that require nothing more than kernel interrupt locking.
 */

static inline bool atomic_cas(atomic_t *target, atomic_val_t old_value,
	atomic_val_t new_value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    bool cmp = (*target == old_value);
    if (cmp)
        *target = new_value;
    TX_RESTORE
    return cmp;
}

static inline bool atomic_ptr_cas(atomic_ptr_t *target, atomic_ptr_val_t old_value,
	atomic_ptr_val_t new_value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    bool cmp = (*target == old_value);
    if (cmp)
        *target = new_value;
    TX_RESTORE
    return cmp;
}

static inline atomic_val_t atomic_add(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    atomic_val_t ret = *target;
    *target += value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_sub(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    atomic_val_t ret = *target;
    *target -= value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_inc(atomic_t *target) {
	return atomic_add(target, 1);
}

static inline atomic_val_t atomic_dec(atomic_t *target) {
	return atomic_sub(target, 1);
}

static inline atomic_val_t atomic_get(const atomic_t *target) {
    return *target;
}

static atomic_ptr_val_t atomic_ptr_get(const atomic_ptr_t *target) {
    return *target;
}

static inline atomic_val_t atomic_set(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    atomic_val_t ret = *target;
    *target = value;
    TX_RESTORE
    return ret;
}

static inline atomic_ptr_val_t atomic_ptr_set(atomic_ptr_t *target, atomic_ptr_val_t value) {
    TX_INTERRUPT_SAVE_AREA
    TX_DISABLE
    atomic_ptr_val_t ret = *target;
    *target = value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_clear(atomic_t *target) {
	return atomic_set(target, 0);
}

static inline atomic_ptr_val_t atomic_ptr_clear(atomic_ptr_t *target) {
	return atomic_ptr_set(target, NULL);
}

static inline atomic_val_t atomic_or(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA

    TX_DISABLE
    atomic_val_t ret = *target;
    *target |= value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_xor(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA
    
    TX_DISABLE
    atomic_val_t ret = *target;
    *target ^= value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_and(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA
    
    TX_DISABLE
    atomic_val_t ret = *target;
    *target &= value;
    TX_RESTORE
    return ret;
}

static inline atomic_val_t atomic_nand(atomic_t *target, atomic_val_t value) {
    TX_INTERRUPT_SAVE_AREA
    
    TX_DISABLE
    atomic_val_t ret = *target;
    *target =  ~(*target & value);
    TX_RESTORE
    return ret;
}

#ifdef __cplusplus
}
#endif
#endif /* SYS_ATOMIC_C_H_ */
