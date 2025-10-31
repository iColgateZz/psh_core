#include "cmd.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "log.h"
#include "macros.h"
#include <time.h>
#include "sb.h"
#include "fd.h"

static inline Proc _cmd_start_proc(Cmd cmd, Fd fdin, Fd fdout, Fd fderr);
static inline b32 _block_unwanted_procs(Procs *async, u8 max_procs);
static inline void _setup_child_io(Fd fdin, Fd fdout, Fd fderr);
static inline b32 _proc_wait(Proc pid);
static inline i32 _proc_wait_async(Proc pid);
static inline b32 _procs_wait(Procs procs);
static inline void _cmd_build_cstr(Cmd cmd, String_Builder *sb);
static inline i32 _nprocs(void);

#define SLEEP_MS 1
#define SLEEP_NS SLEEP_MS * 1000 * 1000

b32 cmd_run_opt(Cmd *cmd, Cmd_Opt opt) {
    b32 result = true;
    Fd fdin = INVALID_FD; 
    Fd fdout = INVALID_FD;
    Fd fderr = INVALID_FD;

    u8 max_procs = opt.max_procs > 0 ? opt.max_procs : _nprocs() + 1;
    if (opt.async) {
        if (!_block_unwanted_procs(opt.async, max_procs)) return false;
    }

    if (opt.fdin) {
        fdin = fd_open_read(opt.fdin);
        if (fdin == INVALID_FD) return_defer(false); 
    }

    if (opt.fdout) {
        fdout = fd_open_write(opt.fdout);
        if (fdout == INVALID_FD) return_defer(false); 
    }

    if (opt.fderr) {
        fderr = fd_open_write(opt.fderr);
        if (fderr == INVALID_FD) return_defer(false); 
    }

    Proc pid = _cmd_start_proc(*cmd, fdin, fdout, fderr);

    if (pid == INVALID_PROC) return_defer(false);

    if (opt.async) {
        da_append(opt.async, pid);
    } else {
        result = _proc_wait(pid);
    }

defer:
    if (fdin != INVALID_FD) fd_close(fdin);
    if (fdout != INVALID_FD) fd_close(fdout);
    if (fderr != INVALID_FD) fd_close(fderr);

    return result;
}

b32 procs_flush(Procs *procs) {
    b32 result = _procs_wait(*procs);
    procs->count = 0;
    return result;
}

static inline Proc _cmd_start_proc(Cmd cmd, Fd fdin, Fd fdout, Fd fderr) {
    
    if (cmd.count < 1) {
        logger(ERROR, "Cannot run an empty command");
        return INVALID_PROC;
    }

#ifndef NO_ECHO
    String_Builder sb = {0};
    _cmd_build_cstr(cmd, &sb);
    logger(INFO, "CMD: %s", sb.items);
    sb_free(sb);
#endif

    Proc cpid = fork();
    if (cpid < 0) {
        logger(ERROR, "Could not fork for child process %s", strerror(errno));
        return INVALID_PROC;
    }

    if (cpid == 0) {
        _setup_child_io(fdin, fdout, fderr);

        cmd_append(&cmd, NULL);
        execvp(cmd.items[0], cmd.items);

        logger(ERROR, "Could not exec child process for %s: %s", cmd.items[0], strerror(errno));
        exit(EXIT_FAILURE);

        UNREACHABLE("_cmd_start_proc");
    }

    return cpid;
}

static inline void _setup_child_io(Fd fdin, Fd fdout, Fd fderr) {
    if (fdin != INVALID_FD) {
        if (dup2(fdin, STDIN_FILENO) < 0) {
            logger(ERROR, "Could not setup stdin for child process: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fdout != INVALID_FD) {
        if (dup2(fdout, STDOUT_FILENO) < 0) {
            logger(ERROR, "Could not setup stdout for child process: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    if (fderr != INVALID_FD) {
        if (dup2(fderr, STDERR_FILENO) < 0) {
            logger(ERROR, "Could not setup stderr for child process: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

static inline b32 _block_unwanted_procs(Procs *async, u8 max_procs) {
    // while loop blocks until the allowed
    // amount of procs is left running
    while (async->count >= max_procs) {
        for (usize i = 0; i < async->count; ) {
            i32 ret = _proc_wait_async(async->items[i]);
            if (ret < 0) 
                return false;
            if (ret) {
                da_remove_unordered(async, i);
            } else {
                ++i;
            }
        }
    }
    return true;
}

static inline b32 _proc_wait(Proc pid) {
    i32 wstatus;

    for (;;) {
        if (waitpid(pid, &wstatus, WUNTRACED) < 0) {
            // Interrupted by signal, retry waitpid
            if (errno == EINTR) continue;

            logger(ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
            return false;
        }

        break;
    }

    if (WIFEXITED(wstatus)) {
        i32 exit_status = WEXITSTATUS(wstatus);
        if (exit_status != EXIT_SUCCESS) 
            logger(ERROR, "command exited with exit code %d", exit_status);
        return exit_status == EXIT_SUCCESS;
    }

    if (WIFSIGNALED(wstatus)) {
        logger(ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return false;
    }

    if (WIFSTOPPED(wstatus)) {
        logger(ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return false;
    }

    UNREACHABLE("_proc_wait");
}

static inline i32 _proc_wait_async(Proc pid) {
    i32 wstatus;

    Proc ret = waitpid(pid, &wstatus, WUNTRACED | WNOHANG);
    if (ret < 0) {
        // Interrupted by signal, will be retried later 
        if (errno == EINTR) 
            return 0;
        logger(ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
        return -1;
    }

    // With WNOHANG if waitpid returns 0, the process has 
    // not yet finished running. Sleep and retry later.
    if (ret == 0) {
        static struct timespec duration = {
            .tv_sec = SLEEP_NS / (1000*1000*1000),
            .tv_nsec = SLEEP_NS % (1000*1000*1000),
        };

        nanosleep(&duration, NULL);
        return 0;
    }

    if (WIFEXITED(wstatus)) {
        i32 exit_status = WEXITSTATUS(wstatus);
        if (exit_status != EXIT_SUCCESS) {
            logger(ERROR, "command exited with exit code %d", exit_status);
            return -1;
        }

        return 1;
    }

    if (WIFSIGNALED(wstatus)) {
        logger(ERROR, "command process was terminated by signal %d", WTERMSIG(wstatus));
        return -1;
    }

    if (WIFSTOPPED(wstatus)) {
        logger(ERROR, "command process was stopped by signal %d", WSTOPSIG(wstatus));
        return -1;
    }

    UNREACHABLE("_proc_wait_async");
}

static inline b32 _procs_wait(Procs procs) {
    b32 result = true;
    for (usize i = 0; i < procs.count; ++i) {
        result = _proc_wait(procs.items[i]);
    }
    return result;
}

static inline void _cmd_build_cstr(Cmd cmd, String_Builder *sb) {
    for (usize i = 0; i < cmd.count; ++i) {
        byte *arg = cmd.items[i];
        if (arg == NULL) return;
        if (i > 0) sb_append(sb, ' ');
        sb_append_cstr(sb, arg);
    }
    sb_append_null(sb);
}

static inline i32 _nprocs(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}
