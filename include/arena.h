#ifndef ARENA_INCLUDE
#define ARENA_INCLUDE

#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/cdefs.h>
#include <string.h>

#include "types.h"

#define arena_alloc(...)                allocx_(__VA_ARGS__, alloc2_, alloc1_)(__VA_ARGS__)
#define allocx_(a, b, c, d, ...)        d
#define alloc1_(a_ptr, t)               arena_alloc_(a_ptr, sizeof(t), alignof(t), 1)
#define alloc2_(a_ptr, t, n)            arena_alloc_(a_ptr, sizeof(t), alignof(t), n)

typedef struct {
    void *cur, *end;
    void *buffer;
} Arena;

typedef struct {
    Arena *arena;
    void *cur;
} ScratchArena;

Arena arena_init(isize alloc_size);
void arena_free(Arena *arena);
void arena_clear(Arena *arena);
void *arena_alloc_(Arena *arena, isize size, isize align, isize n);

// TODO 
// 1. Rework arena_alloc to work with default function arguments struct macro trick
// 2. Add ScratchArena functionality
// 3. Add helper function to check malloc in arena_init
//    or use DI and give buffer as an argument.
// 4. arena_resize? Linked list approach?

#ifdef ARENA_IMPL

Arena arena_init( isize alloc_size )
{
    void *buffer = malloc( alloc_size );

    Arena arena = {
        .buffer = buffer,
        .cur = buffer,
        .end = buffer + alloc_size
    };

    return arena;
}

void arena_free( Arena *arena )
{ free( arena->buffer ); }

void arena_clear( Arena *arena )
{
    memset( arena->buffer, 0, (isize)(arena->cur - arena->buffer) );
    arena->cur = arena->buffer;
}

void *arena_alloc_( Arena *arena, isize size, isize align, isize n )
{
    isize padding = -(uptr)arena->cur & (align - 1);

    isize available = arena->end - arena->cur - padding;
    if (available < 0 || n > available / size) {
        fprintf(stderr, "arena: no memory\n");
        return NULL;
    }

    void *p = arena->cur + padding;
    arena->cur += padding + n * size;

    return p;
}

#endif // ARENA_IMPL

#endif // ARENA_INCLUDE
