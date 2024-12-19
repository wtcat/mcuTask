/*
 * Copyright (c) 2024 wtcat
 */

typedef void (*cpp_init_fn)(void);

extern cpp_init_fn _sinit[];
extern cpp_init_fn _einit[];

void __do_init_array(void) {
    for (cpp_init_fn *fn = _sinit; fn < _einit; fn++)
        (*fn)();
}
