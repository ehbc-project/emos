#ifndef __ASYNC_H__
#define __ASYNC_H__

#include <setjmp.h>

struct promise {
    // struct jmp_buf env;
    int finished;
};

void await(struct promise p);
int await_timeout(struct promise p, int timeout);

#endif // __ASYNC_H__
