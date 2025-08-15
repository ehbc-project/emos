#include <stdio.h>

static FILE _stdout = {
    .fd = 0,
};

static FILE _stdin = {
    .fd = 1,
};

static FILE _stderr = {
    .fd = 2,
};

static FILE _stddbg = {
    .fd = 3,
};

FILE *stdin = &_stdin, *stdout = &_stdout, *stderr = &_stderr, *stddbg = &_stddbg;