#include <stdio.h>
#include "types.h"
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "fun.h"


b32 _fun(Cmd *cmd, i32 n, b32 async) {
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid) {
            pid_t a = waitpid(pid, NULL, async);
            if (a < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return -1;
            }
        } else {
            execvp(cmd->items[0], cmd->items);
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

b32 fun2(Cmd *cmd, _funparams params) {
    return _fun(cmd, params.n, params.async);
}