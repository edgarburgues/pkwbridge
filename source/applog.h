/*
 * applog.h - tiny logger to a fixed file on SD.
 *
 * Named "applog" (not "log") to avoid clashing with libm's log() during
 * include resolution.
 */
#ifndef APPLOG_H
#define APPLOG_H

#include <stdio.h>

extern FILE *g_log;

void applog_open(const char *path);
void applog_close(void);

__attribute__((format(printf, 1, 2)))
void pkw_log(const char *fmt, ...);

#endif
