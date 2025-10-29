#include "fun.h"
#include "types.h"
#include "cmd.h"
#include <stdio.h>

i32 main(void) {
    Cmd cmd = {0};

    cmd_append(&cmd, "echo", "Hello, World", "!");
    cmd_append(&cmd, NULL);
    // da_foreach(byte *, i, &cmd) {
    //     printf("%s\n", *i);
    // }

    fun(&cmd, .n=10);

    return 0;
}
