#include "fd.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

Fd fd_open(byte *path, i32 mode, i32 permissions) {
    Fd result = open(path, mode, permissions);
    if (result < 0) {
        logger(ERROR, "Could not open file %s: %s", path, strerror(errno));
        return INVALID_FD;
    }
    return result;
}

Fd fd_read(byte *path) {
    return fd_open(path, O_RDONLY, 0);
}

Fd fd_write(byte *path) {
    return fd_open(path, 
                   O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

Fd fd_append(byte *path) {
    return fd_open(path, 
                   O_WRONLY | O_CREAT | O_APPEND,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

void fd_close(Fd fd) {
    close(fd);
}