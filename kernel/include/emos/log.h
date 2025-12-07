#ifndef __EMOS_LOG_H__
#define __EMOS_LOG_H__

#include <emos/compiler.h>

#define LOG_TARCE    0
#define LOG_DEBUG    1
#define LOG_INFO     2
#define LOG_WARNING  3
#define LOG_ERROR    4
#define LOG_CRITICAL 5

__format_printf(2, 3)
void emos_log(int level, const char *fmt, ...);

#endif // __EMOS_LOG_H__
