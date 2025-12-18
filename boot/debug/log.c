#include <eboot/log.h>

#include <stdio.h>

#include <eboot/asm/intrinsics/rdtsc.h>

#ifdef NDEBUG
static int log_level = LL_NONE;

#else
static int log_level = LL_DEBUG;

#endif

void log_set_level(int level)
{
    if (level < -1) level = -1;
    if (level > 4) level = 4;

    log_level = level;
}

void log_printf(int level, const char *module_name, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    log_vprintf(level, module_name, fmt, args);
    va_end(args);
}

void log_isr_printf(int level, const char *module_name, const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    log_isr_vprintf(level, module_name, fmt, args);
    va_end(args);
}

static const char *ll_str[] = {
    "FATAL", "ERROR", "WARN", "INFO", "DEBUG",
};

void log_vprintf(int level, const char *module_name, const char *fmt, va_list args)
{
    if (log_level < level) return;

    fprintf(stddbg, "%llu %s [%s] ", _i686_rdtsc(), module_name, ll_str[level]);
    vfprintf(stddbg, fmt, args);
}

void log_isr_vprintf(int level, const char *module_name, const char *fmt, va_list args)
{
    if (log_level < level) return;

    fprintf(stddbg, "%llu %s [%s] ", _i686_rdtsc(), module_name, ll_str[level]);
    vfprintf(stddbg, fmt, args);
}
