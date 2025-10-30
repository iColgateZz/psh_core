#ifndef PROC_INCLUDE
#define PROC_INCLUDE

#include "types.h"

typedef i32 Proc;
#define INVALID_PROC -1

typedef struct {
    Proc *items;
    usize count;
    usize capacity;
} Procs;

typedef i32 Fd;

#endif