#define PSH_CORE_IMPL
#define PSH_CORE_NO_PREFIX
    #include "psh_core.h"

i32 example_simple_command();
i32 example_read_cmd_output();
i32 example_pipeline();
i32 example_multiple_readers();
i32 example_redirect_stderr_to_stdout();

i32 main() {
    // example_simple_command();
    // example_read_cmd_output();
    // example_pipeline();
    // example_multiple_readers();
    example_redirect_stderr_to_stdout();

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

    Fd_Reader reader = {.fd = pipe.read_fd};
    if (!fd_read1(&reader)) return 1;

    logger(PSH_INFO, "I read cmd output: %.*s", sb_arg(reader.store));
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
        pipeline_chain(&p, &cmd, .fdin = fd_openr("test.sh"), .fdout = fd_openw("file.txt"));
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

i32 example_multiple_readers() {
    Unix_Pipe outpipe = {0};
    if (!pipe_open(&outpipe)) return 1;

    Unix_Pipe errpipe = {0};
    if (!pipe_open(&errpipe)) return 1;

    const i32 rcount = 2;
    Fd_Reader readers[rcount] = {
        [0].fd = outpipe.read_fd,
        [1].fd = errpipe.read_fd
    };

    Cmd cmd = {0};
    cmd_append(&cmd, "time", "ls", "-alh");

    if (!cmd_run(
        &cmd, 
        .fdout = outpipe.write_fd,
        .fderr = errpipe.write_fd
    )) return 1;

    for (i32 i = 0; i < 10; ++i) {
        // read whatever is readable
        // in a non-blocking io fashion
        fd_read(readers, rcount, .non_blocking_io = true);
        // do some other stuff
        // ...
        // ...
    }

    if (!fd_readers_join(readers, rcount)) return 1;

    printf("From stdout: %.*s\n", sb_arg(readers[0].store));
    printf("From stderr: %.*s\n", sb_arg(readers[1].store));
    return 0;
}

i32 example_redirect_stderr_to_stdout() {
    Unix_Pipe outpipe = {0};
    if (!pipe_open(&outpipe)) return 1;

    Unix_Pipe errpipe = {0};
    if (!pipe_open(&errpipe)) return 1;

    const i32 rcount = 2;
    Fd_Reader readers[rcount] = {
        [0].fd = outpipe.read_fd,
        [1].fd = errpipe.read_fd
    };

    Cmd cmd = {0};
    // by default time writes its output to stderr
    cmd_append(&cmd, "time", "ls", "-alh");

    if (!cmd_run(
        &cmd, 
        .fdout = outpipe.write_fd,
        .fderr = outpipe.write_fd
    )) return 1;

    close(errpipe.write_fd);
    if (!fd_readers_join(readers, rcount)) return 1;

    printf("From stdout: %.*s\n", sb_arg(readers[0].store));
    printf("From stderr: %.*s\n", sb_arg(readers[1].store));
    return 0;
}