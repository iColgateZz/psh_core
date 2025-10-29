#ifndef DA_INCLUDE
#define DA_INCLUDE

#include <string.h>

#ifndef DA_ASSERT
#include <assert.h>
#define DA_ASSERT assert
#endif

#ifndef DA_REALLOC
#include <stdlib.h>
#define DA_REALLOC realloc
#endif

#ifndef DA_FREE
#include <stdlib.h>
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

#endif