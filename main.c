#define PSH_CORE_IMPL
#define PSH_CORE_NO_PREFIX
    #include "psh_core.h"

i32 example_simple_command();
i32 example_read_cmd_output();
i32 example_pipeline();

i32 main() {
    // example_simple_command();
    example_read_cmd_output();
    // example_pipeline();

    return 0;
}

i32 example_simple_command() {
    Cmd cmd = {0};
    cmd_append(&cmd, "echo", "lol");

    return !cmd_run(&cmd);
}

i32 example_read_cmd_output() {
    Unix_Pipe pipe = {0};
    if (!pipe_open(&pipe)) return 1;

    Cmd cmd = {0};
    cmd_append(&cmd, "echo", "lol", "haha");
    if (!cmd_run(&cmd, .fdout = pipe.write_fd)) return 1;

    SB sb = {0};
    if (!fd_read(pipe.read_fd, &sb)) return 1;
    sb_append_null(&sb);

    logger(PSH_INFO, "I read cmd output: %s", sb.items);
    return 0;
}

i32 example_pipeline() {
    Cmd cmd = {0};
    Procs procs = {0};
    Pipeline p = {0};
    
    pipeline(&p, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "xxd");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "xxd");
        pipeline_chain(&p, &cmd, .fdin = fd_oread("test.sh"), .fdout = fd_owrite("file.txt"));
    } if (p.error) return 1;

    pipeline(&p, .async = &procs) {
        cmd_append(&cmd, "./test.sh");
        pipeline_chain(&p, &cmd);

        cmd_append(&cmd, "wc", "-l");
        pipeline_chain(&p, &cmd);
    } if (p.error) return 1;

    printf("pipeline is async!\n");
    if (!procs_block(&procs)) return 1;

    cmd_append(&cmd, "rm", "file.txt");
    if (!cmd_run(&cmd)) return 1;

    return 0;
}
