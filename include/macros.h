#ifndef MACROS_INCLUDE
#define MACROS_INCLUDE

#include <stddef.h>

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define return_defer(value) do { result = (value); goto defer; } while(0)

#endif