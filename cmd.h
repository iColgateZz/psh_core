#ifndef CMD_INCLUDE
#define CMD_INCLUDE

#include "da.h"
#include "types.h"
#include "proc.h"
#include "fd.h"
#include <unistd.h>

typedef struct {
    byte **items;
    usize count;
    usize capacity;
} Cmd;

typedef struct {
    Procs *async;
    u8 max_procs;
    Fd fdin, fdout, fderr;
    b32 no_reset;
} Cmd_Opt;

typedef struct {
    Procs *async;
    u8 max_procs;
    b32 no_reset;
} Pipeline_Opt;

typedef struct {
    Fd last_read_fd;
    Cmd cmd;
    Cmd_Opt cmd_opt;
    Pipeline_Opt p_opt;
    b32 error;
} Pipeline;

#define cmd_append(cmd, ...)                    \
    da_append_many(cmd,                         \
        ((byte *[]){__VA_ARGS__}),              \
        (sizeof((byte *[]){__VA_ARGS__}) / sizeof(byte *)))

#define cmd_free(cmd) da_free(cmd)

#define cmd_run(cmd, ...)   cmd_run_opt(cmd,    \
            (Cmd_Opt) {.fdin = STDIN_FILENO,    \
                       .fdout = STDOUT_FILENO,  \
                       .fderr = STDERR_FILENO,  \
                       __VA_ARGS__              \
                    })
b32 cmd_run_opt(Cmd *cmd, Cmd_Opt opt);
b32 procs_flush(Procs *procs);

#define pipeline_chain(pipeline, cmd, ...)  \
        pipeline_chain_opt(pipeline, cmd,   \
            (Cmd_Opt) {.fdin = STDIN_FILENO,    \
                       .fdout = STDOUT_FILENO,  \
                       .fderr = STDERR_FILENO,  \
                       __VA_ARGS__              \
                    })
b32 pipeline_chain_opt(Pipeline *p, Cmd *cmd, Cmd_Opt opt);
b32 pipeline_end(Pipeline *p);

#define pipeline(p, ...) \
    for (i32 latch = ((p)->p_opt = (Pipeline_Opt) {__VA_ARGS__}, 1); \
                      latch; latch = 0, pipeline_end(p))

#endif