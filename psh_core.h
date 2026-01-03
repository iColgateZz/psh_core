#ifndef PSH_CORE_INCLUDE
#define PSH_CORE_INCLUDE

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Data types START

typedef uint8_t     u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef int8_t      i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef float       f32;
typedef double      f64;

typedef char        byte;
typedef int32_t     b32;

typedef uintptr_t   uptr;
typedef ptrdiff_t   isize;
typedef size_t      usize;

#define true 1
#define false 0
// Data types END

// assert START

#ifndef PSH_ASSERT
    #include <assert.h>
#define PSH_ASSERT assert
#endif
// assert END

// da START

#ifndef PSH_DA_REALLOC
    #define PSH_DA_REALLOC realloc
#endif

#ifndef PSH_DA_FREE
    #define PSH_DA_FREE free
#endif

#define PSH_DA_INIT_CAP 256

#define psh_da_reserve(da, expected_capacity)                                                  \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = PSH_DA_INIT_CAP;                                              \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = PSH_DA_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));  \
            PSH_ASSERT((da)->items != NULL && "Buy more RAM lol");                          \
        }                                                                                  \
    } while (0)

// Append an item to a dynamic array
#define psh_da_append(da, item)                     \
    do {                                        \
        psh_da_reserve((da), (da)->count + 1);      \
        (da)->items[(da)->count++] = (item);    \
    } while (0)

#define psh_da_free(da) PSH_DA_FREE((da).items)

// Append several items to a dynamic array
#define psh_da_append_many(da, new_items, new_items_count)                                          \
    do {                                                                                        \
        psh_da_reserve((da), (da)->count + (new_items_count));                                      \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                       \
    } while (0)

#define psh_da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

// May be used for cleanup
#define psh_da_resize(da, new_size)         \
    do {                                \
        psh_da_reserve((da), new_size);     \
        (da)->count = (new_size);       \
    } while (0)

#define psh_da_last(da) (da)->items[(PSH_ASSERT((da)->count > 0), (da)->count-1)]

// Replace the element at given index with the last
// element in the array and decrement the item count
#define psh_da_remove_unordered(da, i)                   \
    do {                                             \
        usize j = (i);                               \
        PSH_ASSERT(0 <= j && j < (da)->count);        \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while(0)
// da END

// macros START

#define psh_return_defer(value) do { result = (value); goto defer; } while(0)

#define PSH_UNREACHABLE(message)                            \
    do {                                                \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n",     \
                __FILE__, __LINE__, message);           \
        abort();                                        \
    } while(0)

#define PSH_UNUSED(value) (void) value
// macros END

// psh_logger START

typedef enum {
    PSH_INFO, 
    PSH_WARNING,
    PSH_ERROR,
    PSH_NO_LOGS
} Psh_Log_Level;

void psh_logger(Psh_Log_Level level, byte *fmt, ...);
// psh_logger END

// process START

typedef i32 Psh_Proc;
#define PSH_INVALID_PROC -1

typedef struct {
    Psh_Proc *items;
    usize count;
    usize capacity;
} Psh_Procs;
// process END

// fd START

typedef i32 Psh_Fd;
#define PSH_INVALID_FD -1

Psh_Fd psh_fd_open(byte *path, i32 mode, i32 permissions);
Psh_Fd psh_fd_read(byte *path);
Psh_Fd psh_fd_write(byte *path);
Psh_Fd psh_fd_append(byte *path);
void psh_fd_close(Psh_Fd fd);
// fd END

// cmd START

typedef struct {
    byte **items;
    usize count;
    usize capacity;
} Psh_Cmd;

typedef struct {
    Psh_Procs *async;
    u8 max_procs;
    Psh_Fd fdin, fdout, fderr;
    b32 no_reset;
} Psh_Cmd_Opt;

#define psh_cmd_append(cmd, ...)                    \
    psh_da_append_many(cmd,                         \
        ((byte *[]){__VA_ARGS__}),              \
        (sizeof((byte *[]){__VA_ARGS__}) / sizeof(byte *)))

#define psh_cmd_run(cmd, ...)   psh_cmd_run_opt(cmd,    \
            (Psh_Cmd_Opt) {.fdin = STDIN_FILENO,        \
                       .fdout = STDOUT_FILENO,          \
                       .fderr = STDERR_FILENO,          \
                       __VA_ARGS__                      \
                    })
b32 psh_cmd_run_opt(Psh_Cmd *cmd, Psh_Cmd_Opt opt);
b32 psh_procs_block(Psh_Procs *procs);
// cmd END

// pipeline START

typedef struct {
    Psh_Procs *async;
    u8 max_procs;
    b32 no_reset;
} Psh_Pipeline_Opt;

typedef struct {
    Psh_Fd last_read_fd;
    Psh_Cmd cmd;
    Psh_Cmd_Opt cmd_opt;
    Psh_Pipeline_Opt p_opt;
    b32 error;
} Psh_Pipeline;

#define psh_pipeline_chain(pipeline, cmd, ...)  \
        psh_pipeline_chain_opt(pipeline, cmd,   \
            (Psh_Cmd_Opt) {.fdin = STDIN_FILENO,    \
                       .fdout = STDOUT_FILENO,  \
                       .fderr = STDERR_FILENO,  \
                       __VA_ARGS__              \
                    })
b32 psh_pipeline_chain_opt(Psh_Pipeline *p, Psh_Cmd *cmd, Psh_Cmd_Opt opt);
b32 psh_pipeline_end(Psh_Pipeline *p);

#define psh_pipeline(p, ...) \
    for (i32 psh_latch = ((p)->p_opt = (Psh_Pipeline_Opt) {__VA_ARGS__}, 1); \
                      psh_latch; psh_latch = 0, psh_pipeline_end(p))
// pipeline END

// sb START

typedef struct {
    byte *items;
    usize count;
    usize capacity;
} Psh_String_Builder;

#define psh_sb_append(sb, c) psh_da_append(sb, c)
#define psh_sb_append_buf(sb, buf, size) psh_da_append_many(sb, buf, size)
#define psh_sb_append_cstr(sb, cstr)      \
    do {                              \
        byte *s = (cstr);       \
        usize n = strlen(s);         \
        psh_da_append_many(sb, s, n); \
    } while (0)

#define psh_sb_append_null(sb) psh_da_append(sb, 0)
// sb END

#endif // PSH_CORE_INCLUDE

#ifdef PSH_CORE_IMPL

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

// psh_logger impl START

void psh_logger(Psh_Log_Level level, byte *fmt, ...)
{
    switch (level) {
        case PSH_INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case PSH_WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case PSH_ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case PSH_NO_LOGS: return;
        default:
            PSH_UNREACHABLE("psh_logger");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
// psh_logger impl END

// fd impl START

Psh_Fd psh_fd_open(byte *path, i32 mode, i32 permissions) {
    Psh_Fd result = open(path, mode, permissions);
    if (result < 0) {
        psh_logger(PSH_ERROR, "Could not open file %s: %s", path, strerror(errno));
        return PSH_INVALID_FD;
    }
    return result;
}

Psh_Fd psh_fd_read(byte *path) {
    return psh_fd_open(path, O_RDONLY, 0);
}

Psh_Fd psh_fd_write(byte *path) {
    return psh_fd_open(path, 
                   O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

Psh_Fd psh_fd_append(byte *path) {
    return psh_fd_open(path, 
                   O_WRONLY | O_CREAT | O_APPEND,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void psh_fd_close(Psh_Fd fd) {
    close(fd);
}
// fd IMPL END

// cmd IMPL START

static inline Psh_Proc psh__cmd_start_proc(Psh_Cmd cmd, Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr);
static inline b32 psh__block_unwanted_procs(Psh_Procs *async, u8 max_procs);
static inline void psh__setup_child_io(Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr);
static inline b32 psh__proc_wait(Psh_Proc pid);
static inline i32 psh__proc_wait_async(Psh_Proc pid);
static inline b32 psh__procs_wait(Psh_Procs procs);
static inline void psh__cmd_build_cstr(Psh_Cmd cmd, Psh_String_Builder *sb);
static inline i32 psh__nprocs(void);

b32 psh_cmd_run_opt(Psh_Cmd *cmd, Psh_Cmd_Opt opt) {
    b32 result = true;

    if (opt.fdin == PSH_INVALID_FD)  psh_return_defer(false);
    if (opt.fdout == PSH_INVALID_FD) psh_return_defer(false);
    if (opt.fderr == PSH_INVALID_FD) psh_return_defer(false);

    u8 max_procs = opt.max_procs > 0 ? opt.max_procs : psh__nprocs() + 1;
    if (opt.async) {
        if (!psh__block_unwanted_procs(opt.async, max_procs)) psh_return_defer(false);
    }

    Psh_Proc pid = psh__cmd_start_proc(*cmd, opt.fdin, opt.fdout, opt.fderr);

    if (pid == PSH_INVALID_PROC) psh_return_defer(false);

    if (opt.async) {
        psh_da_append(opt.async, pid);
    } else {
        result = psh__proc_wait(pid);
    }

defer:
    if (opt.fdin > STDERR_FILENO)  psh_fd_close(opt.fdin);
    if (opt.fdout > STDERR_FILENO) psh_fd_close(opt.fdout);
    if (opt.fderr > STDERR_FILENO) psh_fd_close(opt.fderr);

    if (!opt.no_reset) psh_da_resize(cmd, 0);

    return result;
}

b32 psh_procs_block(Psh_Procs *procs) {
    b32 result = psh__procs_wait(*procs);
    procs->count = 0;
    return result;
}

static inline Psh_Proc psh__cmd_start_proc(Psh_Cmd cmd, Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr) {
    
    if (cmd.count < 1) {
        psh_logger(PSH_ERROR, "Cannot run an empty command");
        return PSH_INVALID_PROC;
    }

#ifndef PSH_NO_ECHO
    Psh_String_Builder sb = {0};
    psh__cmd_build_cstr(cmd, &sb);
    psh_logger(PSH_INFO, "CMD: %s", sb.items);
    psh_da_free(sb);
#endif

    Psh_Proc cpid = fork();
    if (cpid < 0) {
        psh_logger(PSH_ERROR, "Could not fork a child process: %s", strerror(errno));
        return PSH_INVALID_PROC;
    }

    if (cpid == 0) {
        psh__setup_child_io(fdin, fdout, fderr);

        psh_cmd_append(&cmd, NULL);
        execvp(cmd.items[0], cmd.items);

        psh_logger(PSH_ERROR, "Could not exec in child process for '%s': %s", cmd.items[0], strerror(errno));
        exit(EXIT_FAILURE);

        PSH_UNREACHABLE("psh__cmd_start_proc");
    }

    return cpid;
}

static inline void psh__setup_child_io(Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr) {
    // psh_logger(PSH_INFO, "Psh_Fds: %d, %d, %d", fdin, fdout, fderr);
    if (fdin != PSH_INVALID_FD) {
        if (dup2(fdin, STDIN_FILENO) < 0) {
            psh_logger(PSH_ERROR, "Could not setup stdin(%d) for child process: %s", fdin, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fdout != PSH_INVALID_FD) {
        if (dup2(fdout, STDOUT_FILENO) < 0) {
            psh_logger(PSH_ERROR, "Could not setup stdout(%d) for child process: %s", fdout, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fderr != PSH_INVALID_FD) {
        if (dup2(fderr, STDERR_FILENO) < 0) {
            psh_logger(PSH_ERROR, "Could not setup stderr(%d) for child process: %s", fderr, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static inline b32 psh__block_unwanted_procs(Psh_Procs *async, u8 max_procs) {
    // while loop blocks until the allowed
    // amount of procs is left running
    while (async->count >= max_procs) {
        for (usize i = 0; i < async->count; ) {
            i32 ret = psh__proc_wait_async(async->items[i]);
            if (ret < 0) 
                return false;
            if (ret) {
                psh_da_remove_unordered(async, i);
            } else {
                ++i;
            }
        }
    }
    return true;
}

static inline b32 psh__proc_wait(Psh_Proc pid) {
    i32 wstatus;

    for (;;) {
        if (waitpid(pid, &wstatus, WUNTRACED) < 0) {
            // Interrupted by signal, retry waitpid
            if (errno == EINTR) continue;

            psh_logger(PSH_ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
            return false;
        }

        break;
    }

    if (WIFEXITED(wstatus)) {
        i32 exit_status = WEXITSTATUS(wstatus);
        if (exit_status != EXIT_SUCCESS) 
            psh_logger(PSH_ERROR, "command exited with exit code %d", exit_status);
        return exit_status == EXIT_SUCCESS;
    }

    if (WIFSIGNALED(wstatus)) {
        psh_logger(PSH_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return false;
    }

    if (WIFSTOPPED(wstatus)) {
        psh_logger(PSH_ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return false;
    }

    PSH_UNREACHABLE("psh__proc_wait");
}

static inline i32 psh__proc_wait_async(Psh_Proc pid) {
    i32 wstatus;

    Psh_Proc ret = waitpid(pid, &wstatus, WUNTRACED | WNOHANG);
    if (ret < 0) {
        // Interrupted by signal, will be retried later 
        if (errno == EINTR) 
            return 0;
        psh_logger(PSH_ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
        return -1;
    }

    // With WNOHANG if waitpid returns 0, the process has 
    // not yet finished running. Sleep and retry later.
    if (ret == 0) {
        #define SLEEP_MS 1
        #define SLEEP_NS SLEEP_MS * 1000 * 1000
        static struct timespec duration = {
            .tv_sec = SLEEP_NS / (1000*1000*1000),
            .tv_nsec = SLEEP_NS % (1000*1000*1000),
        };

        nanosleep(&duration, NULL);
        return 0;
    }

    if (WIFEXITED(wstatus)) {
        i32 exit_status = WEXITSTATUS(wstatus);
        if (exit_status != EXIT_SUCCESS) {
            psh_logger(PSH_ERROR, "command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        psh_logger(PSH_ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return -1;
    }

    if (WIFSTOPPED(wstatus)) {
        psh_logger(PSH_ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return -1;
    }

    PSH_UNREACHABLE("psh__proc_wait_async");
}

static inline b32 psh__procs_wait(Psh_Procs procs) {
    b32 result = true;
    for (usize i = 0; i < procs.count; ++i) {
        result = psh__proc_wait(procs.items[i]);
    }
    return result;
}

static inline void psh__cmd_build_cstr(Psh_Cmd cmd, Psh_String_Builder *sb) {
    for (usize i = 0; i < cmd.count; ++i) {
        byte *arg = cmd.items[i];
        if (arg == NULL) return;
        if (i > 0) psh_sb_append(sb, ' ');
        psh_sb_append_cstr(sb, arg);
    }
    psh_sb_append_null(sb);
}

static inline i32 psh__nprocs(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}
// cmd IMPL END

// pipeline IMPL START

static inline void psh__pipeline_setup_opt(Psh_Cmd_Opt *prev_opt, 
    Psh_Pipeline_Opt pipe_opt, Psh_Fd pipe_fdin, Psh_Fd pipe_fdout);

b32 psh_pipeline_chain_opt(Psh_Pipeline *p, Psh_Cmd *new_cmd, Psh_Cmd_Opt new_cmd_opt) {
    if (p->error) return false;

    // Execute previous cmd
    if (p->cmd.count != 0) {
        Psh_Fd fds[2];
        if (pipe(fds) < 0) {
            p->error = true;
            psh_logger(PSH_ERROR, "Could not create pipes %s", strerror(errno));
            if (p->last_read_fd > STDERR_FILENO) psh_fd_close(p->last_read_fd);
            return false;
        }

        psh__pipeline_setup_opt(&p->cmd_opt, p->p_opt, p->last_read_fd, fds[STDOUT_FILENO]);
        // closes all fds passed to it
        b32 ok = psh_cmd_run_opt(&p->cmd, p->cmd_opt);

        if (!ok) {
            p->error = true;
            psh_fd_close(fds[STDIN_FILENO]);
            return false;
        }

        p->last_read_fd = fds[STDIN_FILENO];
    }

    p->cmd_opt = new_cmd_opt;
    p->cmd = (Psh_Cmd) {0};
    psh_da_append_many(&p->cmd, new_cmd->items, new_cmd->count);

    if (!p->p_opt.no_reset) new_cmd->count = 0;

    return true;
}

b32 psh_pipeline_end(Psh_Pipeline *p) {
    if (p->error) return false;

    psh__pipeline_setup_opt(&p->cmd_opt, p->p_opt, p->last_read_fd, STDOUT_FILENO);
    b32 ok = psh_cmd_run_opt(&p->cmd, p->cmd_opt);

    psh_da_free(p->cmd);
    *p = (Psh_Pipeline) {0};
    return ok;
}

static inline void psh__pipeline_setup_opt(Psh_Cmd_Opt *prev_opt, Psh_Pipeline_Opt pipe_opt,
                                           Psh_Fd pipe_fdin, Psh_Fd pipe_fdout) {
    // If prev_opt has non-default settings it means 
    // the user has opened a file for redirection. 
    // Close the fds from pipes and leave user fds.
    if (prev_opt->fdin != STDIN_FILENO) {
        if (pipe_fdin != STDIN_FILENO)
            psh_fd_close(pipe_fdin);
    } else {
        // Otherwise setup pipe fds
        prev_opt->fdin = pipe_fdin;
    }

    if (prev_opt->fdout != STDOUT_FILENO) {
        if (pipe_fdout != STDOUT_FILENO)
            psh_fd_close(pipe_fdout);
    } else {
        prev_opt->fdout = pipe_fdout;
    }
    
    prev_opt->async = pipe_opt.async;
    prev_opt->max_procs = pipe_opt.max_procs;
}

// pipeline IMPL END

#endif // PSH_CORE_IMPL

#ifdef PSH_CORE_NO_PREFIX

#define da_reserve          psh_da_reserve
#define da_append           psh_da_append
#define da_free             psh_da_free
#define da_append_many      psh_da_append_many
#define da_foreach          psh_da_foreach
#define da_resize           psh_da_resize
#define da_last             psh_da_last
#define da_remove_unordered psh_da_remove_unordered

#define return_defer        psh_return_defer
#define UNREACHABLE         PSH_UNREACHABLE
#define UNUSED              PSH_UNUSED

typedef Psh_Log_Level       Log_Level;
#define INFO                PSH_INFO
#define WARNING             PSH_WARNING
#define ERROR               PSH_ERROR
#define NO_LOGS             PSH_NO_LOGS
#define logger              psh_logger

typedef Psh_Proc            Proc;
#define INVALID_PROC        PSH_INVALID_PROC
typedef Psh_Procs           Procs;

typedef Psh_Fd              Fd;
#define INVALID_FD          PSH_INVALID_FD
#define fd_open             psh_fd_open
#define fd_read             psh_fd_read
#define fd_write            psh_fd_write
#define fd_append           psh_fd_append
#define fd_close            psh_fd_close

typedef Psh_Cmd             Cmd;
typedef Psh_Cmd_Opt         Cmd_Opt;
#define cmd_append          psh_cmd_append
#define cmd_run             psh_cmd_run
#define cmd_run_opt         psh_cmd_run_opt
#define procs_block         psh_procs_block

typedef Psh_Pipeline_Opt    Pipeline_Opt;
typedef Psh_Pipeline        Pipeline;
#define pipeline_chain      psh_pipeline_chain
#define pipeline_chain_opt  psh_pipeline_chain_opt
#define pipeline_end        psh_pipeline_end
#define pipeline            psh_pipeline

typedef Psh_String_Builder  String_Builder;
#define sb_append           psh_sb_append
#define sb_append_buf       psh_sb_append_buf
#define sb_append_cstr      psh_sb_append_cstr
#define sb_append_null      psh_sb_append_null

#endif // PSH_CORE_NO_PREFIX