#include <ctype.h>

#include <stdint.h>

#define CNTRL   0x0001
#define PRINT   0x0002
#define SPACE   0x0004
#define BLANK   0x0008
#define GRAPH   0x0010
#define PUNCT   0x0020
#define ALNUM   0x0040
#define ALPHA   0x0080
#define UPPER   0x0100
#define LOWER   0x0200
#define DIGIT   0x0400
#define XDIGI   0x0800

static const uint16_t ctype_table[128] = {
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,  // 00h-07h
    0x0001, 0x000D, 0x0005, 0x0005, 0x0005, 0x0005, 0x0001, 0x0001,  // 08h-0Fh
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,  // 10h-17h
    0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001,  // 18h-1Fh
    0x000E, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032,  // 20h-27h
    0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032,  // 28h-2Fh
    0x0C52, 0x0C52, 0x0C52, 0x0C52, 0x0C52, 0x0C52, 0x0C52, 0x0C52,  // 30h-37h
    0x0C52, 0x0C52, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032,  // 38h-3Fh
    0x0032, 0x09D2, 0x09D2, 0x09D2, 0x09D2, 0x09D2, 0x09D2, 0x01D2,  // 40h-47h
    0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2,  // 48h-4Fh
    0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2, 0x01D2,  // 50h-57h
    0x01D2, 0x01D2, 0x01D2, 0x0032, 0x0032, 0x0032, 0x0032, 0x0032,  // 58h-5Fh
    0x0032, 0x0AD2, 0x0AD2, 0x0AD2, 0x0AD2, 0x0AD2, 0x0AD2, 0x02D2,  // 60h-67h
    0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2,  // 68h-6Fh
    0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2, 0x02D2,  // 70h-77h
    0x02D2, 0x02D2, 0x02D2, 0x0032, 0x0032, 0x0032, 0x0032, 0x0001,  // 78h-7Fh
};

#undef isalnum
int isalnum(int c) {
#ifndef __HAVE_BUILTIN_ISALNUM
    return !!(ctype_table[c] & ALNUM);

#else
    return __builtin_isalnum(c);

#endif
}

#undef isalpha
int isalpha(int c) {
#ifndef __HAVE_BUILTIN_ISALPHA
    return !!(ctype_table[c] & ALPHA);

#else
    return __builtin_isalpha(c);

#endif
}

#undef iscntrl
int iscntrl(int c) {
#ifndef __HAVE_BUILTIN_ISCNTRL
    return !!(ctype_table[c] & CNTRL);

#else
    return __builtin_iscntrl(c);

#endif
}

#undef isdigit
int isdigit(int c) {
#ifndef __HAVE_BUILTIN_ISDIGIT
    return !!(ctype_table[c] & DIGIT);

#else
    return __builtin_isdigit(c);

#endif
}

#undef isgraph
int isgraph(int c) {
#ifndef __HAVE_BUILTIN_ISGRAPH
    return !!(ctype_table[c] & GRAPH);

#else
    return __builtin_isgraph(c);

#endif
}

#undef islower
int islower(int c) {
#ifndef __HAVE_BUILTIN_ISLOWER
    return !!(ctype_table[c] & LOWER);

#else
    return __builtin_islower(c);

#endif
}

#undef isprint
int isprint(int c) {
#ifndef __HAVE_BUILTIN_ISPRINT
    return !!(ctype_table[c] & PRINT);

#else
    return __builtin_isprint(c);

#endif
}

#undef ispunct
int ispunct(int c) {
#ifndef __HAVE_BUILTIN_ISPUNCT
    return !!(ctype_table[c] & PUNCT);

#else
    return __builtin_ispunct(c);

#endif
}

#undef isspace
int isspace(int c) {
#ifndef __HAVE_BUILTIN_ISSPACE
    return !!(ctype_table[c] & SPACE);

#else
    return __builtin_isspace(c);

#endif
}

#undef isupper
int isupper(int c) {
#ifndef __HAVE_BUILTIN_ISUPPER
    return !!(ctype_table[c] & UPPER);

#else
    return __builtin_isupper(c);

#endif
}

#undef isxdigit
int isxdigit(int c) {
#ifndef __HAVE_BUILTIN_ISXDIGIT
    return !!(ctype_table[c] & XDIGI);

#else
    return __builtin_isxdigit(c);

#endif
}

#undef isblank
int isblank(int c) {
#ifndef __HAVE_BUILTIN_ISBLANK
    return !!(ctype_table[c] & BLANK);

#else
    return __builtin_isblank(c);

#endif
}

#undef toupper
int toupper(int c) {
#ifndef __HAVE_BUILTIN_TOUPPER
    return (ctype_table[c] & LOWER) ? c - 'a' + 'A' : c;

#else
    return __builtin_toupper(c);

#endif
}

#undef tolower
int tolower(int c) {
#ifndef __HAVE_BUILTIN_TOLOWER
    return (ctype_table[c] & UPPER) ? c - 'A' + 'a' : c;

#else
    return __builtin_tolower(c);

#endif
}