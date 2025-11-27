#ifndef PSH_CORE_INCLUDE
#define PSH_CORE_INCLUDE

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

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


#endif // PSH_CORE_IMPL