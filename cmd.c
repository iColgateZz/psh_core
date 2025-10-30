#include "cmd.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "log.h"
#include "macros.h"

static inline Proc _cmd_start_proc(Cmd *cmd, Fd fdin, Fd fdout, Fd fderr);
static inline b32 _cmd_wait_proc(Proc pid);
static inline i32 _cmd_wait_proc_async(Proc pid);

b32 cmd_run_opt(Cmd *cmd, Cmd_Opt opt) {
    b32 result = true;

    // while loop is blocking until the allowed
    // amount of procs is left running
    if (opt.async && opt.max_procs > 0) {
        while (opt.async->count >= opt.max_procs) {
            for (usize i = 0; i < opt.async->count; ++i) {
                i32 ret = _cmd_wait_proc_async(opt.async->items[i]);
                if (ret < 0) 
                    return_defer(false);
                if (ret) {
                    da_remove_unordered(opt.async, i);
                    break;
                }
            }
        }
    }

    Proc pid = _cmd_start_proc(cmd, 0, 0, 0);

    if (pid == INVALID_PROC) return_defer(false);

    if (opt.async) {
        da_append(opt.async, pid);
    } else {
        result = _cmd_wait_proc(pid);
    }

defer:
    return result;
}

static inline Proc _cmd_start_proc(Cmd *cmd, Fd fdin, Fd fdout, Fd fderr) {
    (void) fdin;
    (void) fdout;
    (void) fderr;

    Proc cpid = fork();
    if (cpid < 0) {
        logger(ERROR, "Could not fork for child process %s", strerror(errno));
        return INVALID_PROC;
    }

    if (cpid == 0) {
        cmd_append(cmd, NULL);

        if (execvp(cmd->items[0], cmd->items) < 0) {
            logger(ERROR, "Could not exec child process for %s: %s", cmd->items[0], strerror(errno));
            exit(EXIT_FAILURE);
        }

        UNREACHABLE("_cmd_start_proc");
    }

    return cpid;
}

static inline b32 _cmd_wait_proc(Proc pid) {
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

    UNREACHABLE("_cmd_wait_proc");
}

static inline i32 _cmd_wait_proc_async(Proc pid) {
    i32 wstatus;

    Proc ret = waitpid(pid, &wstatus, WUNTRACED | WNOHANG);
    if (ret < 0) {
        // Do not check for EINTR
        // The function is called in an infinite loop
        logger(ERROR, "could not wait on command (pid %d): %s", pid, strerror(errno));
        return -1;
    }

    if (ret == 0) {
        // With WNOHANG if waitpid returns 0, the process has not finished yet.
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

    UNREACHABLE("_cmd_wait_proc_async");
}

b32 cmd_wait_procs(Procs procs) {
    b32 result = true;
    for (usize i = 0; i < procs.count; ++i) {
        result = _cmd_wait_proc(procs.items[i]);
    }
    return result;
}