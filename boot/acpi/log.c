#include <uacpi/kernel_api.h>

#include <stdio.h>

void uacpi_kernel_log(uacpi_log_level, const uacpi_char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void uacpi_kernel_vlog(uacpi_log_level, const uacpi_char *fmt, uacpi_va_list args)
{
    vfprintf(stderr, fmt, args);
}
