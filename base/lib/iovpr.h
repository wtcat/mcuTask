/*
 * Copyright 2022 wtcat
 */
#ifndef BASE_LIB_IOVPR_H_
#define BASE_LIB_IOVPR_H_

#include <stdarg.h>
#include "base/generic.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*vio_put_char)( int c, void *arg );

int _IO_Vprintf(vio_put_char put_char, void *arg, char const *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
#endif /* BASE_LIB_IOVPR_H_ */
