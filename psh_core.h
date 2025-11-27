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

// da START

#ifndef DA_ASSERT
    #include <assert.h>
#define DA_ASSERT assert
#endif

#ifndef DA_REALLOC
    #define DA_REALLOC realloc
#endif

#ifndef DA_FREE
    #define DA_FREE free
#endif

#define DA_INIT_CAP 256

#define da_reserve(da, expected_capacity)                                                  \
    do {                                                                                   \
        if ((expected_capacity) > (da)->capacity) {                                        \
            if ((da)->capacity == 0) {                                                     \
                (da)->capacity = DA_INIT_CAP;                                              \
            }                                                                              \
            while ((expected_capacity) > (da)->capacity) {                                 \
                (da)->capacity *= 2;                                                       \
            }                                                                              \
            (da)->items = DA_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));  \
            DA_ASSERT((da)->items != NULL && "Buy more RAM lol");                          \
        }                                                                                  \
    } while (0)

// Append an item to a dynamic array
#define da_append(da, item)                     \
    do {                                        \
        da_reserve((da), (da)->count + 1);      \
        (da)->items[(da)->count++] = (item);    \
    } while (0)

#define da_free(da) DA_FREE((da).items)

// Append several items to a dynamic array
#define da_append_many(da, new_items, new_items_count)                                          \
    do {                                                                                        \
        da_reserve((da), (da)->count + (new_items_count));                                      \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                       \
    } while (0)

#define da_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

// May be used for cleanup
#define da_resize(da, new_size)         \
    do {                                \
        da_reserve((da), new_size);     \
        (da)->count = (new_size);       \
    } while (0)

#define da_last(da) (da)->items[(DA_ASSERT((da)->count > 0), (da)->count-1)]

// Replace the element at given index with the last
// element in the array and decrement the item count
#define da_remove_unordered(da, i)                   \
    do {                                             \
        usize j = (i);                               \
        DA_ASSERT(0 <= j && j < (da)->count);        \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while(0)
// da END

// macros START

#define return_defer(value) do { result = (value); goto defer; } while(0)

#define UNREACHABLE(message)                            \
    do {                                                \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n",     \
                __FILE__, __LINE__, message);           \
        abort();                                        \
    } while(0)

#define UNUSED(value) (void) value
// macros END

// logger START

typedef enum {
    INFO, 
    WARNING,
    ERROR,
    NO_LOGS
} Log_Level;

void logger(Log_Level level, byte *fmt, ...);
// logger END

// process START

typedef i32 Proc;
#define INVALID_PROC -1

typedef struct {
    Proc *items;
    usize count;
    usize capacity;
} Procs;
// process END

// fd START

typedef i32 Fd;
#define INVALID_FD -1

Fd fd_open(byte *path, i32 mode, i32 permissions);
Fd fd_read(byte *path);
Fd fd_write(byte *path);
Fd fd_append(byte *path);
void fd_close(Fd fd);
// fd END

// cmd START

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
// cmd END

// pipeline START

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
// pipeline END

// sb START

typedef struct {
    byte *items;
    usize count;
    usize capacity;
} String_Builder;

#define sb_append(sb, c) da_append(sb, c)
#define sb_append_buf(sb, buf, size) da_append_many(sb, buf, size)
#define sb_append_cstr(sb, cstr)      \
    do {                              \
        byte *s = (cstr);       \
        usize n = strlen(s);         \
        da_append_many(sb, s, n); \
    } while (0)

#define sb_append_null(sb) da_append(sb, 0)
#define sb_free(sb) DA_FREE((sb).items)
// sb END

#endif // PSH_CORE_INCLUDE

#define PSH_CORE_IMPL
#ifdef PSH_CORE_IMPL

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

// logger impl START

void logger(Log_Level level, byte *fmt, ...)
{
    switch (level) {
        case INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case NO_LOGS: return;
        default:
            UNREACHABLE("logger");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
// logger impl END

// fd impl START

Fd fd_open(byte *path, i32 mode, i32 permissions) {
    Fd result = open(path, mode, permissions);
    if (result < 0) {
        logger(ERROR, "Could not open file %s: %s", path, strerror(errno));
        return INVALID_FD;
    }
    return result;
}

Fd fd_read(byte *path) {
    return fd_open(path, O_RDONLY, 0);
}

Fd fd_write(byte *path) {
    return fd_open(path, 
                   O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

Fd fd_append(byte *path) {
    return fd_open(path, 
                   O_WRONLY | O_CREAT | O_APPEND,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void fd_close(Fd fd) {
    close(fd);
}
// fd IMPL END

// cmd IMPL START

static inline Proc _cmd_start_proc(Cmd cmd, Fd fdin, Fd fdout, Fd fderr);
static inline b32 _block_unwanted_procs(Procs *async, u8 max_procs);
static inline void _setup_child_io(Fd fdin, Fd fdout, Fd fderr);
static inline b32 _proc_wait(Proc pid);
static inline i32 _proc_wait_async(Proc pid);
static inline b32 _procs_wait(Procs procs);
static inline void _cmd_build_cstr(Cmd cmd, String_Builder *sb);
static inline i32 _nprocs(void);

b32 cmd_run_opt(Cmd *cmd, Cmd_Opt opt) {
    b32 result = true;

    if (opt.fdin == INVALID_FD)  return_defer(false);
    if (opt.fdout == INVALID_FD) return_defer(false);
    if (opt.fderr == INVALID_FD) return_defer(false);

    u8 max_procs = opt.max_procs > 0 ? opt.max_procs : _nprocs() + 1;
    if (opt.async) {
        if (!_block_unwanted_procs(opt.async, max_procs)) return_defer(false);
    }

    Proc pid = _cmd_start_proc(*cmd, opt.fdin, opt.fdout, opt.fderr);

    if (pid == INVALID_PROC) return_defer(false);

    if (opt.async) {
        da_append(opt.async, pid);
    } else {
        result = _proc_wait(pid);
    }

defer:
    if (opt.fdin > STDERR_FILENO)  fd_close(opt.fdin);
    if (opt.fdout > STDERR_FILENO) fd_close(opt.fdout);
    if (opt.fderr > STDERR_FILENO) fd_close(opt.fderr);

    if (!opt.no_reset) da_resize(cmd, 0);

    return result;
}

b32 procs_flush(Procs *procs) {
    b32 result = _procs_wait(*procs);
    procs->count = 0;
    return result;
}

static inline Proc _cmd_start_proc(Cmd cmd, Fd fdin, Fd fdout, Fd fderr) {
    
    if (cmd.count < 1) {
        logger(ERROR, "Cannot run an empty command");
        return INVALID_PROC;
    }

#ifndef NO_ECHO
    String_Builder sb = {0};
    _cmd_build_cstr(cmd, &sb);
    logger(INFO, "CMD: %s", sb.items);
    sb_free(sb);
#endif

    Proc cpid = fork();
    if (cpid < 0) {
        logger(ERROR, "Could not fork for child process %s", strerror(errno));
        return INVALID_PROC;
    }

    if (cpid == 0) {
        _setup_child_io(fdin, fdout, fderr);

        cmd_append(&cmd, NULL);
        execvp(cmd.items[0], cmd.items);

        logger(ERROR, "Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
        exit(EXIT_FAILURE);

        UNREACHABLE("_cmd_start_proc");
    }

    return cpid;
}

static inline void _setup_child_io(Fd fdin, Fd fdout, Fd fderr) {
    // logger(INFO, "Fds: %d, %d, %d", fdin, fdout, fderr);
    if (fdin != INVALID_FD) {
        if (dup2(fdin, STDIN_FILENO) < 0) {
            logger(ERROR, "Could not setup stdin(%d) for child process: %s", fdin, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fdout != INVALID_FD) {
        if (dup2(fdout, STDOUT_FILENO) < 0) {
            logger(ERROR, "Could not setup stdout(%d) for child process: %s", fdout, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fderr != INVALID_FD) {
        if (dup2(fderr, STDERR_FILENO) < 0) {
            logger(ERROR, "Could not setup stderr(%d) for child process: %s", fderr, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static inline b32 _block_unwanted_procs(Procs *async, u8 max_procs) {
    // while loop blocks until the allowed
    // amount of procs is left running
    while (async->count >= max_procs) {
        for (usize i = 0; i < async->count; ) {
            i32 ret = _proc_wait_async(async->items[i]);
            if (ret < 0) 
                return false;
            if (ret) {
                da_remove_unordered(async, i);
            } else {
                ++i;
            }
        }
    }
    return true;
}

static inline b32 _proc_wait(Proc pid) {
    i32 wstatus;

    for (;;) {
        if (waitpid(pid, &wstatus, WUNTRACED) < 0) {
            // Interrupted by signal, retry waitpid
            if (errno == EINTR) continue;

            logger(ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
            return false;
        }

        break;
    }

    if (WIFEXITED(wstatus)) {
        i32 exit_status = WEXITSTATUS(wstatus);
        if (exit_status != EXIT_SUCCESS) 
            logger(ERROR, "command exited with exit code %d", exit_status);
        return exit_status == EXIT_SUCCESS;
    }

    if (WIFSIGNALED(wstatus)) {
        logger(ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return false;
    }

    if (WIFSTOPPED(wstatus)) {
        logger(ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return false;
    }

    UNREACHABLE("_proc_wait");
}

static inline i32 _proc_wait_async(Proc pid) {
    i32 wstatus;

    Proc ret = waitpid(pid, &wstatus, WUNTRACED | WNOHANG);
    if (ret < 0) {
        // Interrupted by signal, will be retried later 
        if (errno == EINTR) 
            return 0;
        logger(ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
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
            logger(ERROR, "command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        logger(ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return -1;
    }

    if (WIFSTOPPED(wstatus)) {
        logger(ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return -1;
    }

    UNREACHABLE("_proc_wait_async");
}

static inline b32 _procs_wait(Procs procs) {
    b32 result = true;
    for (usize i = 0; i < procs.count; ++i) {
        result = _proc_wait(procs.items[i]);
    }
    return result;
}

static inline void _cmd_build_cstr(Cmd cmd, String_Builder *sb) {
    for (usize i = 0; i < cmd.count; ++i) {
        byte *arg = cmd.items[i];
        if (arg == NULL) return;
        if (i > 0) sb_append(sb, ' ');
        sb_append_cstr(sb, arg);
    }
    sb_append_null(sb);
}

static inline i32 _nprocs(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}
// cmd IMPL END


#endif // PSH_CORE_IMPL