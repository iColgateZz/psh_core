#include "types.h"

typedef struct {
    i32 n;
    i32 async;
} _funparams;

#define _fun(argv, ...)    fun2(argv, (_funparams) {.n = 1, __VA_ARGS__})

i32 fun2(byte *argv[], _funparams params);