#ifndef PSH_CORE_INCLUDE
#define PSH_CORE_INCLUDE

#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdalign.h>

//TODO: add platform-agnostic wrappers

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

typedef struct {
    byte *s;
    isize len;
} psh_s8;

#define psh_countof(x)                (usize)(sizeof(x) / sizeof(*(x)))
#define psh_lenof(s)                  psh_countof(s) - 1

#define psh_s8(...)                   s8_(__VA_ARGS__, s8_from_heap, s8_read_only)(__VA_ARGS__)
#define s8_(a, b, c, ...)             c
#define s8_read_only(str)             (psh_s8){.s = str, .len = psh_lenof(str)}
#define s8_from_heap(str, str_len)    (psh_s8){.s = str, .len = str_len}
// Data types END

// assert START

#ifndef PSH_ASSERT
    #include <assert.h>
    #define PSH_ASSERT assert
#endif
// assert END

// da START

#ifndef PSH_LIST_REALLOC
    #define PSH_LIST_REALLOC realloc
#endif

#ifndef PSH_LIST_FREE
    #define PSH_LIST_FREE free
#endif

#define PSH_LIST_INIT_CAP 256

#define psh_list_def(Type)      \
    struct Type ## List {       \
        Type *items;            \
        isize count;            \
        isize capacity;         \
    };

#define List(T) struct T ## List

#define psh_slice_def(Type)     \
    struct Type ## Slice {      \
        Type *items;            \
        isize count;            \
    };

#define Slice(T) struct T ## Slice

#define psh_list_reserve(da, expected_capacity)                                                   \
    do {                                                                                        \
        if ((expected_capacity) > (da)->capacity) {                                             \
            if ((da)->capacity == 0) {                                                          \
                (da)->capacity = PSH_LIST_INIT_CAP;                                               \
            }                                                                                   \
            while ((expected_capacity) > (da)->capacity) {                                      \
                (da)->capacity *= 2;                                                            \
            }                                                                                   \
            (da)->items = PSH_LIST_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items));   \
            PSH_ASSERT((da)->items != NULL && "Buy more RAM lol");                              \
        }                                                                                       \
    } while (0)

// Append an item to a dynamic array
#define psh_list_append(da, item)                     \
    do {                                        \
        psh_list_reserve((da), (da)->count + 1);      \
        (da)->items[(da)->count++] = (item);    \
    } while (0)

#define psh_list_free(da) PSH_LIST_FREE((da).items)

// Append several items to a dynamic array
#define psh_list_append_many(da, new_items, new_items_count)                                          \
    do {                                                                                        \
        psh_list_reserve((da), (da)->count + (new_items_count));                                      \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                       \
    } while (0)

#define psh_list_foreach(Type, it, da) for (Type *it = (da)->items; it < (da)->items + (da)->count; ++it)

// May be used for cleanup
#define psh_list_resize(da, new_size)         \
    do {                                \
        psh_list_reserve((da), new_size);     \
        (da)->count = (new_size);       \
    } while (0)

#define psh_list_clear(da) (da)->count = 0
#define psh_list_last(da) (da)->items[(PSH_ASSERT((da)->count > 0), (da)->count-1)]
#define psh_list_pop(da) (da)->items[(PSH_ASSERT((da)->count > 0), --(da)->count)]

// Replace the element at given index with the last
// element in the array and decrement the item count
#define psh_list_remove_unordered(da, i)                   \
    do {                                             \
        usize j = (i);                               \
        PSH_ASSERT(0 <= j && j < (da)->count);        \
        (da)->items[j] = (da)->items[--(da)->count]; \
    } while(0)
// da END

// hash map START

#ifndef PSH_HASH_MAP_REALLOC
    #define PSH_HASH_MAP_REALLOC realloc
#endif

#ifndef PSH_HASH_MAP_FREE
    #define PSH_HASH_MAP_FREE free
#endif

#ifndef PSH_HASH_MAP_INIT_CAP
    #define PSH_HASH_MAP_INIT_CAP 256
#endif

#ifndef PSH_HASH_MAP_MAX_LOAD_PERCENT
    #define PSH_HASH_MAP_MAX_LOAD_PERCENT 70
#endif

typedef enum {
    PSH_HASH_MAP_ENTRY_EMPTY,
    PSH_HASH_MAP_ENTRY_OCCUPIED,
    PSH_HASH_MAP_ENTRY_DELETED,
} Psh_HashMapEntryState;

typedef struct {
    u64 hash;
    u8 state;
} Psh_HashMapEntryHeader;

#define Psh_HashMap(Key, Value)      struct Psh_ ## Key ## _ ## Value ## _HashMap
#define Psh_HashMapEntry(Key, Value) struct Psh_ ## Key ## _ ## Value ## _HashMapEntry

#define psh_hash_map_def(Key, Value)          \
    Psh_HashMapEntry(Key, Value) {            \
        Psh_HashMapEntryHeader header;        \
        Key key;                              \
        Value value;                          \
    };                                        \
    Psh_HashMap(Key, Value) {                 \
        Psh_HashMapEntry(Key, Value) *items;  \
        isize count;                          \
        isize deleted_count;                  \
        isize capacity;                       \
        b32 fixed_capacity;                   \
        u64 (*key_hash)(Key key);             \
        b32 (*key_equal)(Key a, Key b);       \
    };

u64 psh_hash_bytes(void const *data, usize size);
void *psh__hash_map_resize(void *items, isize old_capacity, isize new_capacity, usize entry_size);

// https://prng.di.unimi.it/splitmix64.c
static inline u64 psh__hash_map_mix(u64 hash) {
    hash ^= hash >> 30;
    hash *= UINT64_C(0xbf58476d1ce4e5b9);
    hash ^= hash >> 27;
    hash *= UINT64_C(0x94d049bb133111eb);
    hash ^= hash >> 31;
    return hash;
}

static inline isize psh__hash_map_index(u64 hash, isize capacity) {
    return (isize)(hash & (u64)(capacity - 1));
}

static inline isize psh__hash_map_step(u64 hash, isize capacity) {
    return (isize)(((hash >> 32) | 1) & (u64)(capacity - 1));
}

// memory_count must be a power of two. This storage is never resized or freed.
#define psh_hash_map_init_with_memory(map, memory, memory_count, hash_function, equal_function) do { \
        (map)->count = 0;                                                                            \
        (map)->deleted_count = 0;                                                                    \
        (map)->items = (memory);                                                                     \
        (map)->capacity = (memory_count);                                                            \
        (map)->fixed_capacity = true;                                                                \
        (map)->key_hash = (hash_function);                                                           \
        (map)->key_equal = (equal_function);                                                         \
    } while (0)

#define psh_hash_map_resize(map, requested_capacity) do {                                \
        PSH_ASSERT(!(map)->fixed_capacity && "Cannot resize a fixed-capacity hash map"); \
        if ((map)->capacity == 0) (map)->capacity = PSH_HASH_MAP_INIT_CAP;               \
        isize psh__new_capacity = (map)->capacity;                                       \
        while (psh__new_capacity < (requested_capacity)) {                               \
            psh__new_capacity *= 2;                                                      \
        }                                                                                \
        (map)->items = psh__hash_map_resize(                                             \
            (map)->items, (map)->capacity, psh__new_capacity, sizeof(*(map)->items)      \
        );                                                                               \
        (map)->capacity = psh__new_capacity;                                             \
        (map)->deleted_count = 0;                                                        \
    } while (0)

#define psh_hash_map_clear(map) do {                                        \
        memset((map)->items, 0, (map)->capacity * sizeof(*(map)->items));   \
        (map)->count = 0;                                                   \
        (map)->deleted_count = 0;                                           \
    } while (0)

// inserting an existing key replaces its value
#define psh_hash_map_insert(map, new_key, new_value) do {                                   \
        if ((map)->capacity == 0) psh_hash_map_resize((map), PSH_HASH_MAP_INIT_CAP);        \
        if (!(map)->fixed_capacity &&                                                       \
            ((map)->count + (map)->deleted_count + 1) * 100 >                               \
            (map)->capacity * PSH_HASH_MAP_MAX_LOAD_PERCENT)                                \
        {                                                                                   \
            isize psh__resize_capacity =                                                    \
                ((map)->count + 1) * 100 > (map)->capacity * PSH_HASH_MAP_MAX_LOAD_PERCENT  \
                    ? (map)->capacity * 2                                                   \
                    : (map)->capacity;                                                      \
            psh_hash_map_resize((map), psh__resize_capacity);                               \
        }                                                                                   \
        u64 psh__hash = psh__hash_map_mix((map)->key_hash(new_key));                        \
        isize psh__index = psh__hash_map_index(psh__hash, (map)->capacity);                 \
        isize psh__step = psh__hash_map_step(psh__hash, (map)->capacity);                   \
        isize psh__target = -1;                                                             \
        isize psh__first_deleted = -1;                                                      \
        isize psh__increment_count = 1;                                                     \
        for (isize psh__probe = 0; psh__probe < (map)->capacity; ++psh__probe) {            \
            u8 psh__state = (map)->items[psh__index].header.state;                          \
            if (psh__state == PSH_HASH_MAP_ENTRY_OCCUPIED) {                                \
                if ((map)->items[psh__index].header.hash == psh__hash &&                    \
                    (map)->key_equal((map)->items[psh__index].key, (new_key))) {            \
                    psh__target = psh__index;                                               \
                    psh__increment_count = 0;                                               \
                    break;                                                                  \
                }                                                                           \
            } else if (psh__state == PSH_HASH_MAP_ENTRY_DELETED) {                          \
                if (psh__first_deleted < 0) psh__first_deleted = psh__index;                \
            } else {                                                                        \
                psh__target = psh__first_deleted >= 0 ? psh__first_deleted : psh__index;    \
                break;                                                                      \
            }                                                                               \
            psh__index = (psh__index + psh__step) & ((map)->capacity - 1);                  \
        }                                                                                   \
        if (psh__target < 0) psh__target = psh__first_deleted;                              \
        PSH_ASSERT(psh__target >= 0 && "Hash map capacity exhausted");                      \
        if ((map)->items[psh__target].header.state == PSH_HASH_MAP_ENTRY_DELETED) {         \
            (map)->deleted_count--;                                                         \
        }                                                                                   \
        (map)->items[psh__target].header.hash = psh__hash;                                  \
        (map)->items[psh__target].header.state = PSH_HASH_MAP_ENTRY_OCCUPIED;               \
        (map)->items[psh__target].key = (new_key);                                          \
        (map)->items[psh__target].value = (new_value);                                      \
        (map)->count += psh__increment_count;                                               \
    } while (0)

#define psh_hash_map_get(map, search_key, result_pointer) do {                    \
        *(result_pointer) = NULL;                                                 \
        if ((map)->capacity <= 0) break;                                          \
        u64 psh__hash = psh__hash_map_mix((map)->key_hash(search_key));           \
        isize psh__index = psh__hash_map_index(psh__hash, (map)->capacity);       \
        isize psh__step = psh__hash_map_step(psh__hash, (map)->capacity);         \
        for (isize psh__probe = 0; psh__probe < (map)->capacity; ++psh__probe) {  \
            u8 psh__state = (map)->items[psh__index].header.state;                \
            if (psh__state == PSH_HASH_MAP_ENTRY_EMPTY) break;                    \
            if (psh__state == PSH_HASH_MAP_ENTRY_OCCUPIED &&                      \
                (map)->items[psh__index].header.hash == psh__hash &&              \
                (map)->key_equal((map)->items[psh__index].key, (search_key))) {   \
                *(result_pointer) = &(map)->items[psh__index].value;              \
                break;                                                            \
            }                                                                     \
            psh__index = (psh__index + psh__step) & ((map)->capacity - 1);        \
        }                                                                         \
    } while (0)

#define psh_hash_map_remove(map, search_key) do {                                       \
        if ((map)->capacity <= 0) break;                                                \
        u64 psh__hash = psh__hash_map_mix((map)->key_hash(search_key));                 \
        isize psh__index = psh__hash_map_index(psh__hash, (map)->capacity);             \
        isize psh__step = psh__hash_map_step(psh__hash, (map)->capacity);               \
        for (isize psh__probe = 0; psh__probe < (map)->capacity; ++psh__probe) {        \
            u8 psh__state = (map)->items[psh__index].header.state;                      \
            if (psh__state == PSH_HASH_MAP_ENTRY_EMPTY) break;                          \
            if (psh__state == PSH_HASH_MAP_ENTRY_OCCUPIED &&                            \
                (map)->items[psh__index].header.hash == psh__hash &&                    \
                (map)->key_equal((map)->items[psh__index].key, (search_key)))           \
            {                                                                           \
                (map)->items[psh__index].header.state = PSH_HASH_MAP_ENTRY_DELETED;     \
                (map)->count--;                                                         \
                (map)->deleted_count++;                                                 \
                if ((map)->count == 0 && (map)->deleted_count * 10 >= (map)->capacity)  \
                    psh_hash_map_clear(map);                                            \
                break;                                                                  \
            }                                                                           \
            psh__index = (psh__index + psh__step) & ((map)->capacity - 1);              \
        }                                                                               \
    } while (0)

#define psh_hash_map_free(map) do { if (!(map)->fixed_capacity) PSH_HASH_MAP_FREE((map)->items); } while(0)

// hash map END

// macros START

#define psh_return_defer(value) do { result = (value); goto defer; } while(0)

#define PSH_UNREACHABLE(message)                            \
    do {                                                \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n",     \
                __FILE__, __LINE__, message);           \
        abort();                                        \
    } while(0)

#define PSH_UNUSED(value) (void) value

#define psh_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define MAX(a, b)           ((a) > (b) ? (a) : (b))
#define MIN(a, b)           ((a) < (b) ? (a) : (b))
#define CLAMP(v, min, max)  MIN(max, MAX(v, min))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
// macros END

// time START

u64 psh_time_now_ns(void);
// time END

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
Psh_Fd psh_fd_openr(byte *path);
Psh_Fd psh_fd_openw(byte *path);
Psh_Fd psh_fd_opena(byte *path);
void psh_fd_close(Psh_Fd fd);
void psh_fd_close_safe(Psh_Fd fd);
b32 psh_fd_not_default(Psh_Fd fd);
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
    b32 keep_fdin_open;
    b32 keep_fdout_open;
    b32 keep_fderr_open;
} Psh_Cmd_Opt;

#define psh_cmd_append(cmd, ...)                    \
    psh_list_append_many(cmd,                         \
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
} Psh_Pipeline_Opt;

typedef struct {
    Psh_Fd prev_read_fd;
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

#define psh_sb_append(sb, c) psh_list_append(sb, c)
#define psh_sb_append_buf(sb, buf, size) psh_list_append_many(sb, buf, size)
#define psh_sb_append_cstr(sb, cstr)      \
    do {                              \
        byte *s = (cstr);       \
        usize n = strlen(s);         \
        psh_list_append_many(sb, s, n); \
    } while (0)

#define psh_sb_append_null(sb) psh_list_append(sb, 0)

typedef Psh_String_Builder Psh_Sb;
#define psh_sb_arg(sb)  (i32)sb.count, sb.items
// sb END

// pipe START

typedef struct {
    Psh_Fd read_fd;
    Psh_Fd write_fd;
} Psh_Unix_Pipe;

b32 psh_pipe_open(Psh_Unix_Pipe *upipe);
// pipe END

// reader START

typedef struct {
    Psh_Fd fd;
    Psh_Sb store;
    b32 ready;
    b32 marked_nb;
} Psh_Fd_Reader;

typedef struct {
    b32 keep_fd_open;
    b32 nonblocking;
} Psh_Fd_Reader_Opt;

b32 psh_fd_read_opt(Psh_Fd_Reader *r, Psh_Fd_Reader_Opt opt);
#define psh_fd_read(reader, ...)     \
    psh_fd_read_opt(reader, (Psh_Fd_Reader_Opt) {__VA_ARGS__})
b32 psh_fd_readers_join(Psh_Fd_Reader r[], usize rcount);
// reader END

// arena START

#define KB(x) ((usize)(x) << 10)
#define MB(x) ((usize)(x) << 20)
#define GB(x) ((usize)(x) << 30)

// Default to 1 gb if arena is zero initialized
#define ARENA_RESERVE_SIZE GB(1)

typedef struct {
    byte* base_ptr;         // Start of the reservation
    usize reserved_size;    // Total size (e.g., 1GB)
    usize committed_size;   // Currently committed memory
    usize current_offset;   // Bump pointer
    usize page_size;        // size of one memory page
} Arena;

typedef usize ArenaSP;

Arena arena_init(usize reserve_size);
void arena_destroy(Arena arena);
void *arena_push_(Arena *arena, usize size, usize align, usize n);
void arena_pop(Arena *arena, usize size);
ArenaSP arena_savepoint(Arena *arena);
void arena_restore(Arena *arena, ArenaSP save_point);
void arena_clear(Arena *arena);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #define alignof_type(T) _Alignof(T)
#else
    // offsetof may also be used?
    #define offsetof_type(T, member) (isize)(&((T *)0)->member)
    #define alignof_type(T)  offsetof_type(struct { char byte; T value; }, value)
#endif

#define arena_push(...)                pushx_(__VA_ARGS__, push2_, push1_)(__VA_ARGS__)
#define pushx_(a, b, c, d, ...)        d
#define push1_(a_ptr, t)               arena_push_(a_ptr, sizeof(t), alignof_type(t), 1)
#define push2_(a_ptr, t, n)            arena_push_(a_ptr, sizeof(t), alignof_type(t), n)

typedef struct {
    Arena *arena;
    ArenaSP sp;
} Scratch;

Scratch scratch_begin(Arena *arena);
void scratch_end(Scratch scratch);

#ifndef ARENA_TEMP_NUM
    #define ARENA_TEMP_NUM 2
#endif

#ifndef ARENA_SCRATCH_SIZE
    #define ARENA_SCRATCH_SIZE MB(8)
#endif

typedef struct {
    Arena temp_arenas[ARENA_TEMP_NUM];
} ThreadCtx;

#ifndef PSH_THREAD_CTX_MOD
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        #define PSH_THREAD_CTX_MOD _Thread_local
    #else
        #define PSH_THREAD_CTX_MOD
    #endif
#endif

PSH_THREAD_CTX_MOD static ThreadCtx thread_ctx = {0};

Scratch scratch_get_(Arena *conflicting_permanent_arenas[], usize conflict_num);
#define scratch_get(...) scratch_get_( \
        ((Arena *[]){__VA_ARGS__}), (sizeof((Arena *[]){__VA_ARGS__}) / sizeof(Arena *)))
// arena END

// unity build START

typedef i32 psh_ternary;
#define psh_err -1

void psh_rebuild_unity(i32 argc, byte *argv[argc], byte *src[], usize src_count);
#define PSH_REBUILD_UNITY(argc, argv, ...)                                      \
        psh_rebuild_unity(argc, argv,  ((byte *[]){__FILE__, __VA_ARGS__}),     \
        (sizeof((byte *[]){__FILE__, __VA_ARGS__}) / sizeof(byte *)));

void psh_rebuild_unity_auto(i32 argc, byte *argv[argc], byte *source);
#define PSH_REBUILD_UNITY_AUTO(argc, argv)                   \
        psh_rebuild_unity_auto(argc, argv, __FILE__);

#define psh_shift(array, array_size) (PSH_ASSERT((array_size) > 0), (array_size)--, *(array)++)

#ifndef PSH_CC
    #define PSH_CC "gcc"
#endif

#ifndef PSH_CC_FLAGS 
    #define PSH_CC_FLAGS  "-Wall", "-Wextra", "-O2", "-Wno-initializer-overrides"
#endif

#ifndef PSH_CC_MORE_FLAGS
    #define PSH_CC_MORE_FLAGS ""
#endif

#ifndef PSH_CC_CMD
    #define PSH_CC_CMD(target, source1, ...) \
            PSH_CC, PSH_CC_FLAGS, PSH_CC_MORE_FLAGS, "-o", target, source1, __VA_ARGS__
#endif
// unity build END

// codepoint START

//TODO: make more memory efficient by encoding
//      raw_len and d_width in 1 byte?
typedef struct {
    byte raw[4];
    u8 raw_len;
    u8 display_width;
} Psh_CodePoint;

#define psh_cp(s)      psh_cp_from_utf8(s, sizeof(s) - 1)
Psh_CodePoint psh_cp_from_utf8(byte *s, u8 len);
Psh_CodePoint psh_cp_from_raw(byte *raw, u8 raw_len, u8 display_width);
Psh_CodePoint psh_cp_from_byte(byte b);
b32 psh_cp_equal(Psh_CodePoint a, Psh_CodePoint b);

typedef u32 Psh_Unicode;
u8 psh_unicode_width(Psh_Unicode ch);

u8  psh_utf8_expected_len(byte first);
Psh_CodePoint  psh_utf8_next(byte **start, byte *end);

static Psh_CodePoint PSH_UTF8_REPLACEMENT = {
    .raw = {0xEF, 0xBF, 0xBD},
    .raw_len = 3,
    .display_width = 1,
};

// codepoint END

#endif // PSH_CORE_INCLUDE

#ifdef PSH_CORE_IMPL

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/poll.h>

// time IMPL START

u64 psh_time_now_ns(void) {
    struct timespec current_time = {0};
    clock_gettime(CLOCK_MONOTONIC, &current_time);
    return current_time.tv_sec * 1000000000 + current_time.tv_nsec;
}
// time IMPL END

// hash map IMPL START

// https://www.rfc-editor.org/rfc/rfc9923.html
u64 psh_hash_bytes(void const *data, usize size) {
    byte const *bytes = data;
    u64 hash = UINT64_C(14695981039346656037);
    for (usize i = 0; i < size; ++i) {
        hash ^= (u8)bytes[i];
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

void *psh__hash_map_resize(void *items, isize old_capacity, isize new_capacity, usize entry_size) {
    byte *new_items = PSH_HASH_MAP_REALLOC(NULL, (usize)new_capacity * entry_size);
    PSH_ASSERT(new_items != NULL && "Could not allocate hash map");
    memset(new_items, 0, (usize)new_capacity * entry_size);

    byte *old_items = items;
    for (isize i = 0; i < old_capacity; ++i) {
        byte *old_entry = old_items + (usize)i * entry_size;
        Psh_HashMapEntryHeader header;
        memcpy(&header, old_entry, sizeof(header));
        if (header.state != PSH_HASH_MAP_ENTRY_OCCUPIED) continue;

        isize index = psh__hash_map_index(header.hash, new_capacity);
        isize step = psh__hash_map_step(header.hash, new_capacity);
        for (isize probe = 0; probe < new_capacity; ++probe) {
            byte *new_entry = new_items + (usize)index * entry_size;
            Psh_HashMapEntryHeader new_header;
            memcpy(&new_header, new_entry, sizeof(new_header));
            if (new_header.state == PSH_HASH_MAP_ENTRY_EMPTY) {
                memcpy(new_entry, old_entry, entry_size);
                break;
            }
            index = (index + step) & (new_capacity - 1);
        }
    }

    PSH_HASH_MAP_FREE(items);
    return new_items;
}

// hash map IMPL END

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

Psh_Fd psh_fd_openr(byte *path) {
    return psh_fd_open(path, O_RDONLY, 0);
}

Psh_Fd psh_fd_openw(byte *path) {
    return psh_fd_open(path, 
                   O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

Psh_Fd psh_fd_opena(byte *path) {
    return psh_fd_open(path, 
                   O_WRONLY | O_CREAT | O_APPEND,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void psh_fd_close(Psh_Fd fd) {
    close(fd);
}

void psh_fd_close_safe(Psh_Fd fd) {
    if (psh_fd_not_default(fd))
        psh_fd_close(fd);
}

b32 psh_fd_not_default(Psh_Fd fd) {
    return fd > STDERR_FILENO;
}
// fd IMPL END

// cmd IMPL START

static inline Psh_Proc psh__cmd_start_proc(Psh_Cmd cmd, Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr);
static inline b32 psh__block_unwanted_procs(Psh_Procs *async, u8 max_procs);
static inline void psh__setup_child_io(Psh_Fd fdin, Psh_Fd fdout, Psh_Fd fderr);
static inline b32 psh__proc_wait(Psh_Proc pid);
static inline i32 psh__proc_wait_async(Psh_Proc pid);
static inline b32 psh__procs_wait(Psh_Procs procs);
static inline void psh__cmd_build_cstr(Psh_Cmd cmd, Psh_Sb *sb);
static inline i32 psh__nprocs(void);

b32 psh_cmd_run_opt(Psh_Cmd *cmd, Psh_Cmd_Opt opt) {
    b32 result = true;

    if (opt.fdin  == PSH_INVALID_FD) psh_return_defer(false);
    if (opt.fdout == PSH_INVALID_FD) psh_return_defer(false);
    if (opt.fderr == PSH_INVALID_FD) psh_return_defer(false);

    u8 max_procs = opt.max_procs > 0 ? opt.max_procs : psh__nprocs() + 1;
    if (opt.async) {
        if (!psh__block_unwanted_procs(opt.async, max_procs)) psh_return_defer(false);
    }

    Psh_Proc pid = psh__cmd_start_proc(*cmd, opt.fdin, opt.fdout, opt.fderr);
    if (pid == PSH_INVALID_PROC) psh_return_defer(false);

    if (opt.async)
        psh_list_append(opt.async, pid);
    else
        result = psh__proc_wait(pid);

defer:
    if (!opt.keep_fdin_open)  psh_fd_close_safe(opt.fdin);
    if (!opt.keep_fdout_open) psh_fd_close_safe(opt.fdout);
    if (!opt.keep_fderr_open) psh_fd_close_safe(opt.fderr);

    cmd->count = 0;

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
    Psh_Sb sb = {0};
    psh__cmd_build_cstr(cmd, &sb);
    psh_logger(PSH_INFO, "CMD: %s", sb.items);
    psh_list_free(sb);
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
    if (dup2(fdin, STDIN_FILENO) < 0) {
        psh_logger(PSH_ERROR, "Could not setup stdin(%d) for child process: %s", fdin, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (dup2(fdout, STDOUT_FILENO) < 0) {
        psh_logger(PSH_ERROR, "Could not setup stdout(%d) for child process: %s", fdout, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (dup2(fderr, STDERR_FILENO) < 0) {
        psh_logger(PSH_ERROR, "Could not setup stderr(%d) for child process: %s", fderr, strerror(errno));
        exit(EXIT_FAILURE);
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
                psh_list_remove_unordered(async, i);
            } else {
                #define SLEEP_MS 1
                #define SLEEP_NS SLEEP_MS * 1000 * 1000
                static struct timespec duration = {
                    .tv_sec = SLEEP_NS / (1000*1000*1000),
                    .tv_nsec = SLEEP_NS % (1000*1000*1000),
                };

                nanosleep(&duration, NULL);
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
    // not yet finished running. Retry later.
    if (ret == 0) return 0;

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

static inline void psh__cmd_build_cstr(Psh_Cmd cmd, Psh_Sb *sb) {
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

static inline void psh__pipeline_setup_opt(Psh_Cmd_Opt *prev_cmd_opt, 
    Psh_Pipeline_Opt p_opt, Psh_Fd p_fdin, Psh_Fd p_fdout);

b32 psh_pipeline_chain_opt(Psh_Pipeline *p, Psh_Cmd *new_cmd, Psh_Cmd_Opt new_cmd_opt) {
    if (p->error) return false;

    // Execute previous cmd
    if (p->cmd.count != 0) {
        Psh_Fd fds[2];
        if (pipe(fds) < 0) {
            p->error = true;
            psh_logger(PSH_ERROR, "Could not create pipes %s", strerror(errno));
            psh_fd_close_safe(p->prev_read_fd);
            return false;
        }

        psh__pipeline_setup_opt(&p->cmd_opt, p->p_opt, p->prev_read_fd, fds[STDOUT_FILENO]);
        // closes all non-default fds passed to it
        b32 ok = psh_cmd_run_opt(&p->cmd, p->cmd_opt);

        if (!ok) {
            p->error = true;
            psh_fd_close(fds[STDIN_FILENO]);
            return false;
        }

        p->prev_read_fd = fds[STDIN_FILENO];
    }

    p->cmd_opt = new_cmd_opt;
    p->cmd = (Psh_Cmd) {0};
    psh_list_append_many(&p->cmd, new_cmd->items, new_cmd->count);

    new_cmd->count = 0;

    return true;
}

b32 psh_pipeline_end(Psh_Pipeline *p) {
    if (p->error) return false;

    psh__pipeline_setup_opt(&p->cmd_opt, p->p_opt, p->prev_read_fd, STDOUT_FILENO);
    b32 ok = psh_cmd_run_opt(&p->cmd, p->cmd_opt);

    psh_list_free(p->cmd);
    *p = (Psh_Pipeline) {0};
    return ok;
}

static inline void psh__pipeline_setup_opt(
    Psh_Cmd_Opt *prev_cmd_opt,
    Psh_Pipeline_Opt p_opt,
    Psh_Fd p_fdin,
    Psh_Fd p_fdout
) {
    // If prev_cmd_opt has non-default settings it means 
    // the user has opened a file for redirection. 
    // Close the fds from pipes and leave user fds.
    if (psh_fd_not_default(prev_cmd_opt->fdin)) {
        psh_fd_close(p_fdin);
    } else { // Otherwise continue with pipe fds
        prev_cmd_opt->fdin = p_fdin;
    }

    if (psh_fd_not_default(prev_cmd_opt->fdout)) {
        psh_fd_close(p_fdout);
    } else {
        prev_cmd_opt->fdout = p_fdout;
    }

    // Pipeline options override only default cmd settings
    // Priority (highest to lowest):
    // cmd_opt > pipeline_opt > default settings
    if (prev_cmd_opt->async == NULL)  prev_cmd_opt->async = p_opt.async;
    if (prev_cmd_opt->max_procs == 0) prev_cmd_opt->max_procs = p_opt.max_procs;
}

// pipeline IMPL END

// pipe IMPL START

b32 psh_pipe_open(Psh_Unix_Pipe *upipe) {
    if (pipe((Psh_Fd *)upipe) < 0) {
        psh_logger(PSH_ERROR, "Could not create pipes: %s", strerror(errno));
        return false;
    }

    return true;
}

// pipe IMPL END

// reader IMPL START

static inline
b32 psh__fd_set_nonblocking(Psh_Fd fd) { 
    i32 flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        psh_logger(PSH_ERROR, "Could not get flags of fd(%d): %s", fd, strerror(errno));
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        psh_logger(PSH_ERROR, "Could not set flags of fd(%d): %s", fd, strerror(errno));
        return false;
    }
    
    return true;
}

b32 psh_fd_read_opt(Psh_Fd_Reader *reader, Psh_Fd_Reader_Opt opt) {
    if (reader->ready) return true;

    if (opt.nonblocking && !reader->marked_nb) {
        if (!psh__fd_set_nonblocking(reader->fd))
            return false;

        reader->marked_nb = true;
    }

    isize n;
    byte buffer[1024];
    while ((n = read(reader->fd, buffer, sizeof buffer)) > 0)
        psh_sb_append_buf(&reader->store, buffer, n);

    if (n == 0) {
        if (!opt.keep_fd_open) psh_fd_close_safe(reader->fd);
        reader->ready = true;
        return true;
    }

    if (errno == EAGAIN || errno == EINTR)
        return true;

    psh_logger(PSH_ERROR, "Could not read fd(%d): %s", reader->fd, strerror(errno));
    return false;
}

static inline
b32 psh__fd_readers_poll(Psh_Fd_Reader readers[], usize rcount, i64 timeout) {
    struct pollfd pfds[rcount];
    usize nfds = 0;
    for (usize i = 0; i < rcount; ++i) {
        if (readers[i].ready) continue;

        pfds[nfds++] = (struct pollfd) {
            .fd = readers[i].fd,   
            .events = POLLIN,
        };
    }

    i32 n = poll(pfds, nfds, timeout);
    if (n < 0) {
        if (errno == EINTR) return true;

        psh_logger(PSH_ERROR, "Could not poll: %s", strerror(errno));
        return false;
    }

    if (n == 0) return true;

    for (usize i = 0; i < rcount; ++i) {
        Psh_Fd_Reader *reader = &readers[i];
        struct pollfd pfd = pfds[i];

        if (reader->ready) continue;

        if (pfd.revents & POLLERR) {
            psh_logger(PSH_ERROR, "A poll error occured");
            return false;
        }

        if (pfd.revents & (POLLIN | POLLHUP)) {
            if (!psh_fd_read(reader, .nonblocking = true))
                return false;
        }
    }

    return true;
}

static inline
b32 psh__fd_readers_ready(Psh_Fd_Reader r[], usize rcount) {
    for (usize i = 0; i < rcount; ++i)
        if (!r[i].ready) return false;

    return true;
}

b32 psh_fd_readers_join(Psh_Fd_Reader r[], usize rcount) {
    while (!psh__fd_readers_ready(r, rcount))
        if (!psh__fd_readers_poll(r, rcount, -1))
            return false;

    return true;
}

// reader IMPL END

// arena IMPL START

// Sources of Info:
// https://andreleite.com/posts/2025/nstl/virtual-memory-arena-allocator
// https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator

#define ALIGN_UP_POW2(n, p) (((usize)(n) + ((usize)(p) - 1)) & (~((usize)(p) - 1)))

usize get_page_size(void) 
{ return sysconf(_SC_PAGESIZE); }

Arena arena_init(usize reserve_size) {
    usize PAGE_SIZE = get_page_size();

    // Align reservation up to the nearest page size
    reserve_size = ALIGN_UP_POW2(reserve_size, PAGE_SIZE);
    void* block = mmap(NULL, reserve_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    PSH_ASSERT(block != MAP_FAILED && "Buy more RAM lol");

    return (Arena) {
        .base_ptr = block,
        .reserved_size = reserve_size,
        .page_size = PAGE_SIZE
    };
}

void *arena_push_(Arena *arena, usize size, usize align, usize n) {
    if (arena->base_ptr == NULL) {
        *arena = arena_init(ARENA_RESERVE_SIZE);
    }

    usize offset = ALIGN_UP_POW2(arena->current_offset, align);
    if (n != 0 && size > (arena->reserved_size - offset) / n) {
        return NULL;
    }

    usize new_offset = offset + size * n;
    if (new_offset > arena->reserved_size) {
        return NULL;
    }

    if (new_offset > arena->committed_size) {
        // Align the required commit size up to the nearest page
        usize new_commit_target = ALIGN_UP_POW2(new_offset, arena->page_size);

        if (new_commit_target > arena->reserved_size) {
            new_commit_target = arena->reserved_size;
        }

        usize commit_size = new_commit_target - arena->committed_size;
        void *commit_start_addr = arena->base_ptr + arena->committed_size;

        if (mprotect(commit_start_addr, commit_size, PROT_READ | PROT_WRITE) != 0) {
            return NULL;
        }

        arena->committed_size = new_commit_target;
    }

    void* memory = arena->base_ptr + offset;
    arena->current_offset = new_offset;

    return memory;
}

void arena_destroy(Arena arena) 
{ munmap(arena.base_ptr, arena.reserved_size); }

void arena_pop(Arena *arena, usize size)
{ arena->current_offset -= size; }

ArenaSP arena_savepoint(Arena *arena)
{ return arena->current_offset; }

void arena_restore(Arena *arena, ArenaSP savepoint)
{ arena->current_offset = savepoint; }

void arena_clear(Arena *arena)
{ arena_restore(arena, 0); }

Scratch scratch_begin(Arena *arena)
{ return (Scratch) { .arena = arena, .sp = arena_savepoint(arena)}; }

void scratch_end(Scratch scratch)
{ arena_restore(scratch.arena, scratch.sp); }

Scratch scratch_get_(Arena *conflicting_permanent_arenas[], usize conflict_num) {
    for (usize temp_idx = 0; temp_idx < ARENA_TEMP_NUM; ++temp_idx) {
        Arena *temp_arena = &thread_ctx.temp_arenas[temp_idx];

        if (temp_arena->reserved_size == 0)
            *temp_arena = arena_init(ARENA_SCRATCH_SIZE);

        b32 conflict = false;
        for (usize conflict_idx = 0; conflict_idx < conflict_num; ++conflict_idx) {
            Arena *conflict_arena = conflicting_permanent_arenas[conflict_idx];

            if (temp_arena == conflict_arena) {
                conflict = true;
                break;
            }
        }

        if (!conflict) return scratch_begin(temp_arena);
    }

    PSH_UNREACHABLE("scratch arena not found");
}
// arena IMPL END

// unity build IMPL START

static inline psh_ternary psh__needs_rebuild(byte *executable, byte *src[], usize src_count);

void psh_rebuild_unity(i32 argc, byte *argv[argc], byte *src[], usize src_count) {
    byte *executable = psh_shift(argv, argc);
    byte *source = src[0];

    psh_ternary needs_rebuild = psh__needs_rebuild(executable, src, src_count);
    if (needs_rebuild == psh_err) exit(EXIT_FAILURE);
    if (needs_rebuild == false) return;

    Psh_Cmd cmd = {0};
    psh_cmd_append(&cmd, PSH_CC_CMD(executable, source));
    if (!psh_cmd_run(&cmd)) exit(EXIT_FAILURE);

    psh_cmd_append(&cmd, executable);
    psh_list_append_many(&cmd, argv, argc);
    if (!psh_cmd_run(&cmd)) exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}

static inline
psh_ternary psh__needs_rebuild(byte *executable, byte *src[], usize src_count) {
    struct stat statbuf = {0};
    if (stat(executable, &statbuf) < 0) {
        // Executable does not exist
        if (errno == ENOENT) return true;

        psh_logger(PSH_ERROR, "could not get info about executable %s: %s", executable, strerror(errno));
        return psh_err;
    }
    u32 exec_mod_time = statbuf.st_mtime;

    for (usize i = 0; i < src_count; ++i) {
        byte *source = src[i];
        if (stat(source, &statbuf) < 0) {
            psh_logger(PSH_ERROR, "could not get info about source %s: %s", source, strerror(errno));
            return psh_err;
        }

        u32 source_mod_time = statbuf.st_mtime;
        if (source_mod_time > exec_mod_time) return true;
    }

    return false;
}

typedef struct {
    byte **items;
    usize count;
    usize capacity;
} Sources;

Sources psh__tokenize_deps(usize len, byte string[len]);
b32 psh__is_ws(byte c);
b32 psh__is_bs(byte c);
b32 psh__is_ws_or_bs(byte c);
b32 psh__is_alpha(byte c);
b32 psh__is_num(byte c);
b32 psh__is_path_symbol(byte c);
b32 psh__is_path(byte c);

void psh_rebuild_unity_auto(i32 argc, byte *argv[argc], byte *source) {
    Psh_Unix_Pipe pipe = {0};
    if (!psh_pipe_open(&pipe)) exit(EXIT_FAILURE);

    Psh_Cmd cmd = {0};
    psh_cmd_append(&cmd, "gcc", "-MM", source);
    if (!psh_cmd_run(&cmd, .fdout = pipe.write_fd)) exit(EXIT_FAILURE);
    psh_list_free(cmd);

    Psh_Fd_Reader reader = {.fd = pipe.read_fd};
    if (!psh_fd_read(&reader)) exit(EXIT_FAILURE);

    Sources sources = psh__tokenize_deps(reader.store.count, reader.store.items);
    psh_rebuild_unity(argc, argv, sources.items, sources.count);

    psh_list_free(reader.store);
    psh_list_free(sources);
}

Sources psh__tokenize_deps(usize len, byte string[len]) {
    usize position = 0;
    Sources sources = {0};

    // skip until and over ':'
    while (string[position] != ':' && position < len) ++position;
    ++position;

    while (position < len) {
        while (psh__is_ws_or_bs(string[position]) && position < len) ++position;

        usize start = position;
        while (psh__is_path(string[position]) && position < len) ++position;

        psh_list_append(&sources, string + start);
        string[position] = 0;
        ++position;
        // printf("Dep: %s\n", string + start);
    }

    return sources;
}

b32 psh__is_ws(byte c) {
    return c == ' '  || c == '\n' ||
           c == '\t' || c == '\r' ;
}

b32 psh__is_bs(byte c) {
    return c == '\\';
}

b32 psh__is_ws_or_bs(byte c) {
    return psh__is_ws(c) || psh__is_bs(c);
}

b32 psh__is_alpha(byte c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ;
}

b32 psh__is_num(byte c) {
    return '0' <= c && c <= '9';
}

b32 psh__is_path_symbol(byte c) {
    return c == '.' || c == '/' || 
           c == '_' || c == '-' ;
}

b32 psh__is_path(byte c) {
    return psh__is_path_symbol(c) ||
           psh__is_alpha(c)       ||
           psh__is_num(c)         ;
}
// unity build IMPL END

// codepoint IMPL START

Psh_CodePoint psh_cp_from_utf8(byte *s, u8 len) {
    return  psh_utf8_next(&s, s + len);
}

Psh_CodePoint psh_cp_from_raw(byte *raw, u8 raw_len, u8 display_width) {
    Psh_CodePoint cp = {
        .raw_len = raw_len,
        .display_width = display_width,
    };

    memcpy(cp.raw, raw, raw_len);
    return cp;
}

Psh_CodePoint psh_cp_from_byte(byte b) {
    return (Psh_CodePoint) {
        .raw = {b},
        .raw_len = 1,
        .display_width = 1,
    };
}

b32 psh_cp_equal(Psh_CodePoint a, Psh_CodePoint b) {
    if (a.raw_len != b.raw_len) return false;
    if (a.display_width != b.display_width) return false;
    return memcmp(a.raw, b.raw, a.raw_len) == 0;
}

u8 psh_utf8_expected_len(byte first) {
    u8 b = (u8)first;
    if (b < 0x80) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;
}

Psh_CodePoint psh_utf8_next(byte **p, byte *end) {
    byte *s = *p;

    if (s >= end) {
        return PSH_UTF8_REPLACEMENT;
    }

    u8 first = s[0];
    if (first < 0x80) {
        *p += 1;
        return psh_cp_from_byte(first);
    }

    usize len = 0;
    Psh_Unicode value = 0;

    if ((first & 0xE0) == 0xC0) {
        len = 2;
        value = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        len = 3;
        value = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        len = 4;
        value = first & 0x07;
    } else {
        *p += 1;
        return PSH_UTF8_REPLACEMENT;
    }

    if (s + len > end) {
        *p = end; // consume rest
        return PSH_UTF8_REPLACEMENT;
    }

    for (usize i = 1; i < len; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            *p += i;
            return PSH_UTF8_REPLACEMENT;
        }
        value = (value << 6) | (s[i] & 0x3F);
    }

    Psh_CodePoint cp = psh_cp_from_raw(s, len, psh_unicode_width(value));
    *p += len;
    return cp;
}

u8 psh_unicode_width(Psh_Unicode ch) {
    if (ch < 32 || (ch >= 0x7F && ch < 0xA0))
        return 0;

    // combining marks
    if ((ch >= 0x0300 && ch <= 0x036F) ||
        (ch >= 0x1AB0 && ch <= 0x1AFF) ||
        (ch >= 0x1DC0 && ch <= 0x1DFF) ||
        (ch >= 0x20D0 && ch <= 0x20FF) ||
        (ch >= 0xFE20 && ch <= 0xFE2F))
        return 0;

    // wide characters (CJK + emoji)
    if ((ch >= 0x1100 && ch <= 0x115F) ||
        (ch >= 0x2329 && ch <= 0x232A) ||
        (ch >= 0x2E80 && ch <= 0xA4CF) ||
        (ch >= 0xAC00 && ch <= 0xD7A3) ||
        (ch >= 0xF900 && ch <= 0xFAFF) ||
        (ch >= 0xFE10 && ch <= 0xFE19) ||
        (ch >= 0xFE30 && ch <= 0xFE6F) ||
        (ch >= 0xFF00 && ch <= 0xFF60) ||
        (ch >= 0xFFE0 && ch <= 0xFFE6) ||
        (ch >= 0x1F300 && ch <= 0x1F64F) ||
        (ch >= 0x1F900 && ch <= 0x1F9FF))
        return 2;

    return 1;
}
// codepoint IMPL END

#endif // PSH_CORE_IMPL

#ifdef PSH_CORE_NO_PREFIX

#define s8                      psh_s8
#define countof                 psh_countof
#define lenof                   psh_lenof

#define list_def                psh_list_def
#define list_reserve            psh_list_reserve
#define list_append             psh_list_append
#define list_free               psh_list_free
#define list_append_many        psh_list_append_many
#define list_foreach            psh_list_foreach
#define list_resize             psh_list_resize
#define list_clear              psh_list_clear
#define list_last               psh_list_last
#define list_pop                psh_list_pop
#define list_remove_unordered   psh_list_remove_unordered

#define HashMap                     Psh_HashMap
#define HashMapEntry                Psh_HashMapEntry
#define hash_map_def                psh_hash_map_def
#define hash_map_init_with_memory   psh_hash_map_init_with_memory
#define hash_map_resize             psh_hash_map_resize
#define hash_map_insert             psh_hash_map_insert
#define hash_map_get                psh_hash_map_get
#define hash_map_remove             psh_hash_map_remove
#define hash_map_clear              psh_hash_map_clear
#define hash_map_free               psh_hash_map_free
#define hash_bytes                  psh_hash_bytes

#define slice_def               psh_slice_def

#define return_defer            psh_return_defer
#define UNREACHABLE             PSH_UNREACHABLE
#define UNUSED                  PSH_UNUSED
#define container_of            psh_container_of

#define time_now_ns             psh_time_now_ns

typedef Psh_Log_Level           Log_Level;
#define INFO                    PSH_INFO
#define WARNING                 PSH_WARNING
#define ERROR                   PSH_ERROR
#define NO_LOGS                 PSH_NO_LOGS
#define logger                  psh_logger

typedef Psh_Proc                Proc;
#define INVALID_PROC            PSH_INVALID_PROC
typedef Psh_Procs               Procs;

typedef Psh_Fd                  Fd;
#define INVALID_FD              PSH_INVALID_FD
#define fd_open                 psh_fd_open
#define fd_openr                psh_fd_openr
#define fd_openw                psh_fd_openw
#define fd_opena                psh_fd_opena
#define fd_close                psh_fd_close
#define fd_close_safe           psh_fd_close_safe
#define fd_not_default          psh_fd_not_default

typedef Psh_Cmd                 Cmd;
typedef Psh_Cmd_Opt             Cmd_Opt;
#define cmd_append              psh_cmd_append
#define cmd_run                 psh_cmd_run
#define cmd_run_opt             psh_cmd_run_opt
#define procs_block             psh_procs_block

typedef Psh_Pipeline_Opt        Pipeline_Opt;
typedef Psh_Pipeline            Pipeline;
#define pipeline_chain          psh_pipeline_chain
#define pipeline_chain_opt      psh_pipeline_chain_opt
#define pipeline_end            psh_pipeline_end
#define pipeline                psh_pipeline

#define pipe_open               psh_pipe_open
typedef Psh_Unix_Pipe           Unix_Pipe;
#define fd_read                 psh_fd_read
#define fd_read_opt             psh_fd_read_opt
typedef Psh_Fd_Reader           Fd_Reader;
typedef Psh_Fd_Reader_Opt       Fd_Reader_Opt;
#define fd_readers_join         psh_fd_readers_join

typedef Psh_String_Builder      String_Builder;
typedef Psh_Sb                  Sb;
#define sb_append               psh_sb_append
#define sb_append_buf           psh_sb_append_buf
#define sb_append_cstr          psh_sb_append_cstr
#define sb_append_null          psh_sb_append_null
#define sb_arg                  psh_sb_arg

#define rebuild_unity           psh_rebuild_unity
#define REBUILD_UNITY           PSH_REBUILD_UNITY
#define rebuild_unity_auto      psh_rebuild_unity_auto
#define REBUILD_UNITY_AUTO      PSH_REBUILD_UNITY_AUTO
#define shift                   psh_shift
#define CC                      PSH_CC
#define CC_FLAGS                PSH_CC_FLAGS
#define CC_MORE_FLAGS           PSH_CC_MORE_FLAGS
#define CC_CMD                  PSH_CC_CMD

typedef Psh_CodePoint CodePoint;
#define cp                      psh_cp
#define cp_from_utf8            psh_cp_from_utf8
#define cp_from_raw             psh_cp_from_raw
#define cp_from_byte            psh_cp_from_byte
#define cp_equal                psh_cp_equal
typedef Psh_Unicode Unicode;
#define unicode_width           psh_unicode_width
#define utf8_expected_len       psh_utf8_expected_len
#define utf8_next               psh_utf8_next
#define UTF8_REPLACEMENT PSH_UTF8_REPLACEMENT

#endif // PSH_CORE_NO_PREFIX
