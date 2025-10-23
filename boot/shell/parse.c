#include "shell.h"

#include <stdlib.h>

#define PUSH_CHAR(buf, buflen, ch) \
    do { \
        if (buflen > 0) { \
            *buf++ = ch; \
            buflen--; \
        } else { \
            return NULL; \
        } \
    } while (0)

const char *shell_parse(const char *__restrict line, char *__restrict buf, long buflen)
{
    int escape = 0;
    char quote_char = 0;

    while (*line == ' ' || *line == '\t') {
        line++;
    }

    if (!*line) return NULL;

    while (*line) {
        switch (*line) {
            case '"':
            case '\'':
                quote_char = quote_char == *line ? 0 : *line;
                line++;
                break;
            case '\\':
                escape = 1;
                break;
            case ' ':
            case '\t':
                if (quote_char) {
                    PUSH_CHAR(buf, buflen, *line++);
                } else {
                    goto end;
                }
                break;
            default:
                PUSH_CHAR(buf, buflen, *line++);
                break;
        }

        if (escape) {
            switch (*++line) {
                case '\\':
                case '\'':
                case '\"':
                case '?':
                case ' ':
                case '\t':
                    PUSH_CHAR(buf, buflen, *line++);
                    break;
                case 'a':
                    PUSH_CHAR(buf, buflen, '\a');
                    break;
                case 'b':
                    PUSH_CHAR(buf, buflen, '\b');
                    break;
                case 'e':
                    PUSH_CHAR(buf, buflen, '\033');
                    break;
                case 'f':
                    PUSH_CHAR(buf, buflen, '\f');
                    break;
                case 'n':
                    PUSH_CHAR(buf, buflen, '\n');
                    break;
                case 'r':
                    PUSH_CHAR(buf, buflen, '\r');
                    break;
                case 't':
                    PUSH_CHAR(buf, buflen, '\t');
                    break;
                case 'v':
                    PUSH_CHAR(buf, buflen, '\v');
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7': {
                    char *newptr = (char *)line;
                    PUSH_CHAR(buf, buflen, strtol(line, &newptr, 8));
                    line = newptr;
                    break;
                }
                case 'x': {
                    line++;
                    char *newptr = (char *)line;
                    PUSH_CHAR(buf, buflen, strtol(line, &newptr, 16));
                    line = newptr;
                    break;
                }
            }

            escape = 0;
        }
    }
    
end:
    if (buflen > 0) {
        *buf = 0;
    }

    return line;
}
