#ifndef SB_INCLUDE
#define SB_INCLUDE

#include "types.h"
#include "da.h"
#include <string.h>

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


#endif