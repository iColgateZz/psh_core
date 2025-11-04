#include "types.h"
#include "cmd.h"
#include <stdio.h>
#include "proc.h"

i32 main(void) {
    Cmd cmd = {0};
    Procs procs = {0};
    Pipeline p = {0};

    b32 status;
    pipeline_scope(&p, status, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        if (!pipeline_chain(&p, &cmd)) return 1;

        cmd_append(&cmd, "xxd");
        if (!pipeline_chain(&p, &cmd)) return 1;
        
        cmd_append(&cmd, "xxd");
        if (!pipeline_chain(&p, &cmd)) return 1;
    } if (!status) return 1;

    pipeline_scope(&p, status, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        if (!pipeline_chain(&p, &cmd)) return 1;

        cmd_append(&cmd, "wc", "-l");
        if (!pipeline_chain(&p, &cmd)) return 1;
    } if (!status) return 1;

    printf("pipeline is async!\n");

    if (!procs_flush(&procs)) return 1;

    return 0;
}
