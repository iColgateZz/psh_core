#ifndef LOG_INCLUDE
#define LOG_INCLUDE

#include "types.h"

typedef enum {
    INFO, 
    WARNING,
    ERROR,
    NO_LOGS
} Log_Level;

void logger(Log_Level level, byte *fmt, ...);

#endif
