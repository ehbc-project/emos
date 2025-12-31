#include <emos/thread.h>

#include <stdlib.h>
#include <string.h>

#include <emos/asm/thread.h>
#include <emos/asm/isr.h>

static void real_thread_entry(void)
{
    thread_entry_t entry;
    struct thread *th;

    asm volatile (
        "mov %%ebx, %0\n\t"
        "mov %%ecx, %1\n\t"
        : "=m"(th), "=m"(entry)
    );

    entry(th);

    th->status = TS_FINISHED;
}

status_t _pc_thread_prepare_stack(struct thread *th, size_t stack_size, thread_entry_t entry, void **stack_base_out, void **stack_ptr)
{
    status_t status;
    void *stack_start = NULL;
    struct isr_regs *iregs;
    struct interrupt_frame *iframe;
    uintptr_t esp;

    /* allocate thread stack */
    stack_start = malloc(stack_size);
    if (!stack_start) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    esp = (uintptr_t)stack_start + stack_size;

    /* fill initial interrupt stack frame */
    esp -= sizeof(*iframe);
    iframe = (void *)esp;
    memset(iframe, 0, sizeof(*iframe));
    iframe->eflags = 0x00010206;
    iframe->cs = 0x0008;
    iframe->eip = (uintptr_t)real_thread_entry;

    /* fill initial register stack */
    esp -= sizeof(*iregs);
    iregs = (void *)esp;
    memset(iregs, 0, sizeof(*iregs));
    iregs->ebx = (uintptr_t)th;
    iregs->ecx = (uintptr_t)entry;
    iregs->ds = 0x0010;
    iregs->es = 0x0010;
    iregs->fs = 0x0010;
    iregs->gs = 0x0010;

    esp -= 4;

    if (stack_base_out) *stack_base_out = stack_start;
    if (stack_ptr) *stack_ptr = (void *)esp;

    return STATUS_SUCCESS;

has_error:
    if (stack_start) {
        free(stack_start);
    }

    return status;
}
