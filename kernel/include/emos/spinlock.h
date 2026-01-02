#ifndef __EMOS_SPINLOCK_H__
#define __EMOS_SPINLOCK_H__

#include <emos/thread.h>
#include <emos/status.h>

struct spinlock {
    volatile int locked;
    struct thread *owner;
};

status_t spinlock_init(struct spinlock *lock);

status_t spinlock_lock(struct spinlock *lock);
status_t spinlock_try_lock(struct spinlock *lock);
status_t spinlock_unlock(struct spinlock *lock);

status_t spinlock_try_lock_irqsave(struct spinlock *lock, uint32_t *irqstate);
status_t spinlock_unlock_irqrestore(struct spinlock *lock, uint32_t irqstate);

#endif // __EMOS_SPINLOCK_H__
