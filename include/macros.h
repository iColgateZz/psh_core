#ifndef MACROS_INCLUDE
#define MACROS_INCLUDE

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define return_defer(value) do { result = (value); goto defer; } while(0)

#define UNREACHABLE(message)                            \
    do {                                                \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n",     \
                __FILE__, __LINE__, message);           \
        abort();                                        \
    } while(0)

#define UNUSED(value) (void) value

#endif