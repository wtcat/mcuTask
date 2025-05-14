/*
 * Copyright 2025 wtcat
 */

#include <stdlib.h>
#include <string.h>

#include <base/symbol.h>

LINKER_ROSET(llext_const_symbol, struct llext_const_symbol);

static int compare(const void *s1, const void *s2) {
    return strcmp(s1, s2);
}

const struct llext_const_symbol* symbol_search(const char *name) {
    const struct llext_const_symbol *symtbl;
    size_t n;

    symtbl = LINKER_SET_BEGIN(llext_const_symbol);
    n = LINKER_SET_END(llext_const_symbol) - symtbl;

    return bsearch(name, symtbl, n, sizeof(*symtbl), compare);
}
