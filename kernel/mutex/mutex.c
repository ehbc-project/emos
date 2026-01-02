#include <emos/mutex.h>

#include <emos/scheduler.h>

status_t mutex_init(struct mutex *mtx)
{
    mtx->locked = 0;
    mtx->owner = NULL;
    
    return STATUS_SUCCESS;
}

status_t mutex_lock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    while (mtx->locked) {
        thread_enable_preemption();

        th->status = TS_BLOCKING;
        scheduler_yield();

        thread_disable_preemption();
    }

    mtx->locked = 1;
    mtx->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t mutex_try_lock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;
    
    thread_disable_preemption();

    if (mtx->locked) {
        thread_enable_preemption();

        return STATUS_MUTEX_LOCKED;
    }
    
    mtx->locked = 1;
    mtx->owner = th;

    thread_enable_preemption();

    return STATUS_SUCCESS;
}

status_t mutex_unlock(struct mutex *mtx)
{
    status_t status;
    struct thread *th;

    status = scheduler_get_current_thread(&th);
    if (!CHECK_SUCCESS(status)) return status;

    thread_disable_preemption();

    if (mtx->owner != th) {
        thread_enable_preemption();

        return STATUS_INVALID_THREAD;
    }

    mtx->locked = 0;
    mtx->owner = NULL;
    
    thread_enable_preemption();

    return STATUS_SUCCESS;
}
