#include "types.h"
#include "cmd.h"
#include <stdio.h>
#include "proc.h"

i32 main(void) {
    Cmd cmd = {0};
    Procs procs = {0};

    cmd_append(&cmd, "./test.sh");
    if (!cmd_run(&cmd)) return 1;

    cmd_append(&cmd, "echo", "Hello, World!");
    if (!cmd_run(&cmd, .no_reset = true)) return 1;
    if (!cmd_run(&cmd, .no_reset = true)) return 1;

    // if (!procs_flush(&procs)) return 1;

    return 0;
}
