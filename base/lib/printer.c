/*
 * Copyright 2022 wtcat
 */

#ifdef CONFIG_HEADER_FILE
#include CONFIG_HEADER_FILE
#endif

#include <assert.h>
#include <stdio.h>

#include "base/lib/printer.h"
#include "base/lib/iovpr.h"

#define LOG_MAX_PRINTER 1


struct printer* __log_default_printer;
