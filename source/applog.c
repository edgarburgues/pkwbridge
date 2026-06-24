#include "applog.h"

#include <stdarg.h>

FILE *g_log = NULL;

void applog_open(const char *path) {
    g_log = fopen(path, "w");
}

void applog_close(void) {
    if (g_log) { fclose(g_log); g_log = NULL; }
}

void pkw_log(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap; va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fflush(g_log);
}
