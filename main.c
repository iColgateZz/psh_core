#include "types.h"
#include "cmd.h"
#include <stdio.h>
#include "proc.h"

i32 main(void) {
    Cmd cmd = {0};
    Procs procs = {0};

    cmd_append(&cmd, "./test.sh");

    for (i32 i = 0; i < 100; ++i) {
        if (!cmd_run(&cmd, .async = &procs)) return 1;
    }

    if (!procs_flush(&procs)) return 1;

    return 0;
}
