#ifndef FD_INCLUDE
#define FD_INCLUDE

#include "types.h"

typedef i32 Fd;
#define INVALID_FD -1

Fd fd_open_read(byte *path);
Fd fd_open_write(byte *path);
void fd_close(Fd fd);

#endif
