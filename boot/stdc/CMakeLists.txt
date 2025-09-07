cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE abs.c atoi.c ctype.c errno.c fclose.c fopen.c fprintf.c fputs.c fseek.c ftell.c memcmp.c memcpy.c memset.c printf.c putc.c putchar.c puts.c rand.c snprintf.c sprintf.c stdio.c strchr.c strcspn.c strerror.c strncasecmp.c strncat.c strncmp.c strncpy.c strnlen.c strrchr.c strtok.c strtol.c strtoll.c strtoul.c strtoull.c vprintf.c vsnprintf.c vsprintf.c freopen.c fread.c fwrite.c vfprintf.c wcwidth.c)
