#ifndef CMD_INCLUDE
#define CMD_INCLUDE

#include "da.h"
#include "types.h"
#include "proc.h"

typedef struct {
    byte **items;
    usize count;
    usize capacity;
} Cmd;

typedef struct {
    Procs *async;
    u8 max_procs;
    byte *fdin;
    byte *fdout;
    byte *fderr;
} Cmd_Opt;

#define cmd_append(cmd, ...)                    \
    da_append_many(cmd,                         \
        ((byte *[]){__VA_ARGS__}),              \
        (sizeof((byte *[]){__VA_ARGS__}) / sizeof(byte *)))

#define cmd_free(cmd) da_free(cmd)

#define cmd_run(cmd, ...)   cmd_run_opt(cmd, (Cmd_Opt) {__VA_ARGS__})
b32 cmd_run_opt(Cmd *cmd, Cmd_Opt opt);

b32 procs_wait(Procs procs);

#endif