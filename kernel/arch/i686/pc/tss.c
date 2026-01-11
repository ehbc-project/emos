#include <emos/asm/pc_tss.h>

#include <string.h>

#include <emos/asm/pc_gdt.h>

struct tss _pc_tss;

void _pc_tss_init(void)
{
    memset(&_pc_tss, 0, sizeof(_pc_tss));
    _pc_tss.ss0 = SEG_SEL_KERNEL_DATA;
    _pc_tss.iomap_base = sizeof(_pc_tss);
}

void _pc_tss_set_stack(uintptr_t kstack)
{
    _pc_tss.esp0 = kstack;
}
