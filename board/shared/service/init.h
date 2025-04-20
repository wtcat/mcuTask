/*
 * Copyright 2025 wtcat
 */
#ifndef SERVICE_INIT_H_
#define SERVICE_INIT_H_

#include <base/linker.h>

#ifdef __cplusplus
extern "C"{
#endif

/*
 * System intitialize 
 */

struct sysinit_item {
   int (*handler)(void);
   const char *name;
};

/* 
 * System initialize level (must be <= 99)
 */
#define SI_EARLY_LEVEL         5
#define SI_MEMORY_LEVEL        10
#define SI_PREDRIVER_LEVEL     60
#define SI_BUSDRIVER_LEVEL     70
#define SI_DRIVER_LEVEL        80
#define SI_FILESYSTEM_LEVEL    90
#define SI_APPLICATION_LEVEL   99

#define SYSINIT(_handler, _level, _order) \
    __SYSINIT(_handler, _level, _order)

#define __SYSINIT(_handler, _level, _order) \
    ___SYSINIT(_handler, 0x##_level##_order)

#define ___SYSINIT(_handler, _order) \
    enum { __enum_##_handler = _order}; \
    static LINKER_ROSET_ITEM_ORDERED(sysinit, struct sysinit_item, \
        _handler, _order) = { \
        .handler = _handler, \
        .name = #_handler \
   }

void do_sysinit(void);

#ifdef __cplusplus
}
#endif
#endif /* SERVICE_INIT_H_ */
