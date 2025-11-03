#include "types.h"
#include "cmd.h"
#include <stdio.h>
#include "proc.h"

i32 main(void) {
    Cmd cmd = {0};
    Pipeline p = {0};

    cmd_append(&cmd, "ls");
    if (!pipeline_chain(&p, &cmd)) return 1;

    cmd_append(&cmd, "wc", "-l");
    if (!pipeline_chain(&p, &cmd)) return 1;

    cmd_append(&cmd, "wc", "-l");
    if (!pipeline_chain(&p, &cmd)) return 1;

    if (!pipeline_end(&p)) return 1;

    return 0;
}
