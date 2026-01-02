#ifndef __EMOS_MUTEX_H__
#define __EMOS_MUTEX_H__

#include <emos/thread.h>
#include <emos/status.h>

struct mutex {
    volatile int locked;
    struct thread *owner;
};

status_t mutex_init(struct mutex *mtx);

status_t mutex_lock(struct mutex *mtx);
status_t mutex_lock_with_timeout(struct mutex *mtx, int timeout_ms);
status_t mutex_try_lock(struct mutex *mtx);
status_t mutex_unlock(struct mutex *mtx);

#endif // __EMOS_MUTEX_H__
