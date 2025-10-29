#include "types.h"
#include "cmd.h"

typedef struct {
    i32 n;
    i32 async;
} _funparams;

#define fun(cmd, ...)    fun2(cmd, (_funparams) {.n = 1, __VA_ARGS__})

b32 fun2(Cmd *cmd, _funparams params);