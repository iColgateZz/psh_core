#ifndef LOG_INCLUDE
#define LOG_INCLUDE

#include "types.h"

#define UNREACHABLE(message)                            \
    do {                                                \
        fprintf(stderr, "%s:%d: UNREACHABLE: %s\n",     \
                __FILE__, __LINE__, message);           \
        abort();                                        \
    } while(0)

typedef enum {
    INFO, 
    WARNING,
    ERROR,
    NO_LOGS
} Log_Level;

void logger(Log_Level level, byte *fmt, ...);

#endif
