cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE abs.c atoi.c ctype.c errno.c fprintf.c memcmp.c memcpy.c memset.c printf.c putchar.c rand.c snprintf.c sprintf.c stdio.c strchr.c strerror.c strncasecmp.c strncat.c strncmp.c strncpy.c strnlen.c strrchr.c strtok.c strtol.c strtoll.c vprintf.c vsnprintf.c vsprintf.c freopen.c fread.c fwrite.c vfprintf.c strcspn.c)
