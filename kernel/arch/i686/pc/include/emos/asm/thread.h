#ifndef __EMOS_ASM_THREAD_H__
#define __EMOS_ASM_THREAD_H__

#include <emos/thread.h>

status_t _pc_thread_prepare_stack(struct thread *th, size_t stack_size, thread_entry_t entry, void **stack_base, void **stack_ptr);

#define thread_prepare_stack _pc_thread_prepare_stack

#endif // __EMOS_ASM_THREAD_H__
