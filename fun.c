#include <stdio.h>
#include "types.h"
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include "fun.h"

// f(n, async, command )
i32 fun(byte *argv[], i32 n, b32 async) {
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }

        if (pid) {
            pid_t a = waitpid(pid, NULL, async);
            if (a < 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                return -1;
            }
        } else {
            execvp(argv[0], argv);
            fprintf(stderr, "%s\n", strerror(errno));
            return -1;
        }
    }

    return 0;
}

i32 fun2(byte *argv[], _funparams params) {
    return fun(argv, params.n, params.async);
}