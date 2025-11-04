#include "fd.h"
#include "log.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

Fd fd_read(byte *path) {
    Fd result = open(path, O_RDONLY);
    if (result < 0) {
        logger(ERROR, "Could not open file %s: %s", path, strerror(errno));
        return INVALID_FD;
    }
    return result;
}

Fd fd_write(byte *path) {
    Fd result = open(path,
                     O_WRONLY | O_CREAT | O_TRUNC,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (result < 0) {
        logger(ERROR, "could not open file %s: %s", path, strerror(errno));
        return INVALID_FD;
    }
    return result;
}

void fd_close(Fd fd) {
    close(fd);
}