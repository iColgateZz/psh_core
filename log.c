#include "log.h"
#include <stdarg.h>
#include <stdio.h>

void logger(Log_Level level, byte *fmt, ...)
{
    switch (level) {
        case INFO:
            fprintf(stderr, "[INFO] ");
            break;
        case WARNING:
            fprintf(stderr, "[WARNING] ");
            break;
        case ERROR:
            fprintf(stderr, "[ERROR] ");
            break;
        case NO_LOGS: return;
        default:
            UNREACHABLE("logger");
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}