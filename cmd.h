#ifndef CMD_INCLUDE
#define CMD_INCLUDE

#include "da.h"
#include "types.h"

typedef struct {
    byte **items;
    usize count;
    usize capacity;
} Cmd;

#define cmd_append(cmd, ...)                    \
    da_append_many(cmd,                         \
        ((byte *[]){__VA_ARGS__}),              \
        (sizeof((byte *[]){__VA_ARGS__}) / sizeof(byte *)))

#define cmd_free(cmd) da_free(cmd)

#endif