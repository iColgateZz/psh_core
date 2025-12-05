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

A command `Psh_Cmd` is just a dynamic array of `char *` args:  
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
psh_cmd_run(&cmd, .async = &procs);
psh_cmd_run(&cmd, .async = &procs);
psh_cmd_run(&cmd, .async = &procs);

// the pids of spawned processes are
// stored in the procs array

// later block until all commands 
// finish running
if (!psh_procs_block(&procs)) {
    /* handle error */
}
```

Options for `psh_cmd_run`:
- `.fdin`, `.fdout`, `.fderr` — type: `Psh_Fd` (`int`), redirect standard IO streams, read more about `Psh_Fd` in the File Descriptors section
- `.async`        — type: `Psh_Procs *`, used for non-blocking launch  
- `.max_procs`    — type: `uint8_t`, limit the amount of concurrent async processes  
- `.no_reset`     — type: `1 or 0` ,do not reset the size of given `Psh_Cmd` after running a command

## Pipelines

Chain multiple commands with UNIX-style pipes:
```c
Pipeline p = {0};
for (int _ = (p.p_opt = (Psh_Pipeline_Opt){ .async=NULL, .max_procs=0, .no_reset=false },1);
     _; _ = 0, psh_pipeline_end(&p))
{
    // echo "foo\nbar" | grep f | sort
    psh_pipeline_chain(&p, &echo_cmd, .fdout=STDOUT_FILENO);
    psh_pipeline_chain(&p, &grep_cmd, .fdout=STDOUT_FILENO);
    psh_pipeline_chain(&p, &sort_cmd, .fdout=STDOUT_FILENO);
}
```
Or simply:
```c
psh_pipeline(&p, .async=&procs);
   pipeline_chain(&p, &cmd1);
   pipeline_chain(&p, &cmd2);
   pipeline_chain(&p, &cmd3);
```
When the loop ends, `psh_pipeline_end(&p)` runs the final stage and cleans up. Pipe fds and redirects are handled automatically.

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