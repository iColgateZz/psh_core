#define PSH_CORE_IMPL
#define PSH_CORE_NO_PREFIX
    #include "psh_core.h"

i32 main(void) {
    Cmd cmd = {0};
    Procs procs = {0};
    Pipeline p = {0};
    
    pipeline(&p, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "xxd");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "xxd");
        pipeline_chain(&p, &cmd, .fdin = fd_read("test.sh"), .fdout = fd_write("file.txt"));
    } if (p.error) return 1;

    pipeline(&p, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "wc", "-l");
        pipeline_chain(&p, &cmd);
    } if (p.error) return 1;

    printf("pipeline is async!\n");
    if (!procs_block(&procs)) return 1;

    return 0;
}
