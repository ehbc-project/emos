#include <stdio.h>

static FILE _stdout = {
    .fs = NULL,
    .file = NULL,
    .dev = NULL,
    .charif = NULL,
};

static FILE _stdin = {
    .fs = NULL,
    .file = NULL,
    .dev = NULL,
    .charif = NULL,
};

static FILE _stderr = {
    .fs = NULL,
    .file = NULL,
    .dev = NULL,
    .charif = NULL,
};

static FILE _stddbg = {
    .fs = NULL,
    .file = NULL,
    .dev = NULL,
    .charif = NULL,
};

FILE *stdin = &_stdin, *stdout = &_stdout, *stderr = &_stderr, *stddbg = &_stddbg;