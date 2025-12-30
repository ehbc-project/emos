#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <eboot/status.h>

struct symbol {
    const char *name;
    void *ptr;
};

extern const struct symbol _exported_symbols[];

status_t _module_find_eboot_symbol(const char *name, void **addrout);

#endif // __SYMBOL_H__
