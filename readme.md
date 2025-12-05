# Psh Core: A Lightweight Library for Running Commands and Pipelines  in C

## Introduction

Psh Core is a minimal, single-header C library for:  
- spawning external processes  
- building and running shell-like command pipelines  
- simple file descriptor management  

## Quick Start

1. Drop `psh_core.h` into your project.  
2. Include library's implementation in exactly one C file:
```c
#define PSH_CORE_IMPL
#include "psh_core.h"
```
In other files where you need the API, you can simply do:
```c
#include "psh_core.h"
```
3. In your code:
```c
int main(void) {
    Psh_Cmd cmd = {0};
    psh_cmd_append(&cmd, "ls", "-l");

    if (!psh_cmd_run(&cmd)) {
        // handle error
    }

    return 0;
}
```

And you’re ready to go. Compile and run the `ls -l` process!

## Running Commands

- Declare a `Psh_Cmd` struct:  
```c
Psh_Cmd cmd = {0};
```
- Append arguments:  
```c
// Note: The library automatically adds 
// the NULL terminator required by execvp.
psh_cmd_append(&cmd, "grep", "foo");
```
- Run synchronously (blocks until command completion):
```c
if (!psh_cmd_run(&cmd)) { 
    /* handle error */
}
// Running psh_cmd_run resets the cmd struct
// unless the 'no_reset' option is used. 
// You can safely reuse cmd for other commands
```
- Or run asynchronously using a `Psh_Procs` dynamic array:
```c
Psh_Procs procs = {0};
Psh_Cmd cmd1 = {0};
// Declare and fill other cmds...

// launch several commands asynchronously
psh_cmd_run(&cmd1, .async = &procs);
psh_cmd_run(&cmd2, .async = &procs);
psh_cmd_run(&cmd3, .async = &procs);

// the PIDs of spawned processes are
// stored in the procs array

// later block until all commands 
// in the array finish running
if (!psh_procs_block(&procs)) {
    /* handle error */
}
```

Options for `psh_cmd_run`:
- `Psh_Fd`: `.fdin`, `.fdout`, `.fderr` — redirect standard IO streams, read more about `Psh_Fd` in the File Descriptors section
- `Psh_Procs *`: `.async`        —  used for non-blocking launch  
- `uint8_t`: `.max_procs`    — limit the amount of concurrent async processes. Default is system core count + 1
- `b32`: `.no_reset` — if `true`, the `Psh_Cmd` struct's arguments will *not* be cleared after running the command, allowing for easy reuse with its current arguments. Default is `false`.

## Pipelines

- Declare a `Psh_Pipeline` struct:  
```c
Psh_Pipeline p = {0};
```
- Set up and run a pipeline:
```c
psh_pipeline(&p) {
    // Chain commands
    psh_cmd_append(&cmd, "echo", "foo\nbar\nbaz");
    psh_pipeline_chain(&p, &cmd);

    psh_cmd_append(&cmd, "grep", "b");
    psh_pipeline_chain(&p, &cmd);

    psh_cmd_append(&cmd, "sort");
    psh_pipeline_chain(&p, &cmd);
} if (p.error) {
    // handle error
}
```
Each command in the pipeline is executed immediately, not at the end of the pipeline scope, when all commands are known.

Options for `psh_pipeline`:  
- `Psh_Procs *`: `.async` - non-blocking launch of pipeline
- `uint8_t`: `.max_procs` - limit the amount of concurrent async processes in the pipeline. Default is system core count + 1
- `b32`: `.no_reset` — if `true`, the `Psh_Cmd` struct's arguments will *not* be cleared after each stage, allowing its arguments to persist. Default is `false`.

`pipeline_chain` accepts the same options as `psh_cmd_run`. This way, each command in the pipeline can be customized. However, `.async`, `.max_procs`, and `.no_reset` properties set in the `psh_pipeline` call **override** any corresponding properties set via `psh_pipeline_chain` for individual commands within that pipeline.


## File Descriptors

Convenience functions to open/close file descriptors:
- `Psh_Fd psh_fd_open(char *path, int mode, int permissions)`: Opens a file with specified mode and permissions.
- `Psh_Fd psh_fd_read(char *path)`: Opens a file for reading (`O_RDONLY`).
- `Psh_Fd psh_fd_write(char *path)`: Opens a file for writing, creates if not exists, truncates if exists (`O_WRONLY | O_CREAT | O_TRUNC`).
- `Psh_Fd psh_fd_append(char *path)`: Opens a file for appending, creates if not exists (`O_WRONLY | O_CREAT | O_APPEND`).
- `void psh_fd_close(Psh_Fd fd)`: Closes a file descriptor.  
`psh_fd_open`, `psh_fd_read`, `psh_fd_write`, and `psh_fd_append` functions can fail. In that case, they return `PSH_INVALID_FD` and log the error using `psh_logger(PSH_ERROR, ...)`.


## Logging

Use `psh_logger(level, fmt, ...)` to emit messages:
- Levels: `PSH_INFO`, `PSH_WARNING`, `PSH_ERROR`, `PSH_NO_LOGS`.  
- Example:
```c
psh_logger(PSH_INFO,    "Starting backup process");
psh_logger(PSH_ERROR,   "Failed to open config: %s", strerror(errno));
```

## Customization via Macros

- Define `PSH_NO_ECHO` before including the library to disable the `CMD: ...` output that `psh_cmd_run` prints to `stderr`.
- Define `PSH_CORE_NO_PREFIX` to expose a shorter, un-prefixed API (e.g. `cmd_run` instead of `psh_cmd_run`, `logger` instead of `psh_logger`).

## Future Enhancements

- Improved pipeline control (background jobs, join/kill)  
- More flags for behaviour control and fine-tuning

## License

MIT License. See `LICENSE` for details.
