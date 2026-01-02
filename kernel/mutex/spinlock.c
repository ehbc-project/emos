#include <emos/spinlock.h>

#include <emos/scheduler.h>
#include <emos/asm/interrupt.h>

extern int _pc_irq_level;

status_t spinlock_init(struct spinlock *lock)
{
    lock->locked = 0;
    lock->owner = NULL;

    return STATUS_SUCCESS;
}

status_t spinlock_lock(struct spinlock *lock)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    while (lock->locked) {
        /* busy wait */
    }

    lock->locked = 1;
    lock->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t spinlock_try_lock(struct spinlock *lock)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    if (lock->locked) {
        thread_enable_preemption();
        return STATUS_MUTEX_LOCKED;
    }

    lock->locked = 1;
    lock->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t spinlock_unlock(struct spinlock *lock)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    if (lock->owner != th) {
        thread_enable_preemption();
        return STATUS_INVALID_THREAD;
    }

    lock->locked = 0;
    lock->owner = NULL;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t spinlock_try_lock_irqsave(struct spinlock *lock, uint32_t *irqstate)
{
    status_t status;
    struct thread *th;

    *irqstate = interrupt_save();
    interrupt_disable();

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) {
        interrupt_restore(*irqstate);
        return status;
    }

    if (lock->locked) {
        interrupt_restore(*irqstate);
        return STATUS_MUTEX_LOCKED;
    }

    lock->locked = 1;
    lock->owner = th;

    return STATUS_SUCCESS;
}

status_t spinlock_unlock_irqrestore(struct spinlock *lock, uint32_t irqstate)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    if (lock->owner != th) {
        interrupt_restore(irqstate);
        return STATUS_INVALID_THREAD;
    }

    lock->locked = 0;
    lock->owner = NULL;

    interrupt_restore(irqstate);

    return STATUS_SUCCESS;
}
