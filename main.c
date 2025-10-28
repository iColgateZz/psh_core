#include "fun.h"
#include "types.h"
#include "da.h"
#include <stdio.h>

typedef struct {
    byte **items;
    isize count;
    isize capacity;
} Cmd;

i32 main(void) {
    Cmd cmd = {0};

    da_append(&cmd, "LOL");
    da_append_many(&cmd, ((byte *[]){"haha", "lol", "cringe"}), 3);

    da_foreach(byte *, iter, &cmd) {
        printf("%s\n", *iter);
    }

    da_free(cmd);
    return 0;
}
