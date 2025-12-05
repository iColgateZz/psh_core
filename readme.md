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
psh_cmd_append(&cmd, "grep", "foo");
```
- Run synchronously (blocks until command completion):
```c
if (!psh_cmd_run(&cmd)) { 
    /* handle error */
}
```
- Or run asynchronously using a `Psh_Procs` dynamic array:
```c
Psh_Procs procs = {0};

// launch several
psh_cmd_run(&cmd1, .async = &procs);
psh_cmd_run(&cmd2, .async = &procs);
psh_cmd_run(&cmd3, .async = &procs);

// the pids of spawned processes are
// stored in the procs array

// later block until all commands 
// finish running
if (!psh_procs_block(&procs)) {
    /* handle error */
}
```

Options for `psh_cmd_run`:
- `Psh_Fd`: `.fdin`, `.fdout`, `.fderr` — redirect standard IO streams, read more about `Psh_Fd` in the File Descriptors section
- `Psh_Procs *`: `.async`        —  used for non-blocking launch  
- `uint8_t`: `.max_procs`    — limit the amount of concurrent async processes  
- `bool`: `.no_reset`     — do not reset the size of given `Psh_Cmd` after running a command

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
- `uint8_t`: `.max_procs` - limit the amount of concurrent async processes  
- `bool`: `.no_reset`     — do not reset the sizes of `Psh_Cmd`-s in the pipeline

`pipeline_chain` chain accepts the same options `psh_cmd_run` accepts. This way each command in the pipeline can be customized. However, `.async`, `.max_procs`, and `.no_reset` properties set in the `psh_pipeline` call override the corresponding properties of each command launched in the pipeline.


File Descriptors
----------------
Convenience functions to open/close fds:
- `Psh_Fd psh_fd_read(path)`  
- `Psh_Fd psh_fd_write(path)` (truncate/create)  
- `Psh_Fd psh_fd_append(path)`  
- `void   psh_fd_close(fd)`  
All report errors via `psh_logger(PSH_ERROR,…)` and return `PSH_INVALID_FD` on failure.

Logging
-------
Use `psh_logger(level, fmt, ...)` to emit messages:
- Levels: `PSH_INFO`, `PSH_WARNING`, `PSH_ERROR`, `PSH_NO_LOGS`.  
- Example:
```c
psh_logger(PSH_INFO,    "Starting backup process");
psh_logger(PSH_ERROR,   "Failed to open config: %s", strerror(errno));
```

Customization & Prefix-less API
-------------------------------
Define `PSH_NO_LOGS` before including to disable all logging.  
Define `PSH_CORE_NO_PREFIX` to expose a shorter, un-prefixed API (e.g. `cmd_run` instead of `psh_cmd_run`).

Future Enhancements
-------------------
- Improved pipeline control (background jobs, join/kill)  

License
-------
MIT License. See `LICENSE` for details.