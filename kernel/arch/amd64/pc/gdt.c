#include <emos/asm/pc_gdt.h>

#include <string.h>

#include <emos/asm/pc_tss.h>
#include <emos/asm/intrinsics/gdt.h>
#include <emos/asm/intrinsics/gdt.h>
#include <emos/asm/intrinsics/ltr.h>

#include <emos/compiler.h>

struct gdt_entry _pc_gdt[GDT_ENTRY_COUNT];
static struct gdtr _pc_gdtr;

extern struct tss _pc_tss;

static void gdt_load(void) {
    asm volatile(
        "ljmp %1, $1f\n\t"
        "1:\n\t"
        "mov %0, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : : "i"(SEG_SEL_KERNEL_DATA), "i"(SEG_SEL_KERNEL_CODE) : "memory", "ax"
    );
}

static void set_gdt_entry(int idx, uint32_t base, uint32_t limit, uint8_t access, uint32_t flags)
{
    _pc_gdt[idx].base_low = base & 0xFFFF;
    _pc_gdt[idx].base_mid = (base & 0xFF0000) >> 16;
    _pc_gdt[idx].base_high = (base & 0xFF000000) >> 24;

    _pc_gdt[idx].limit_low = limit & 0xFFFF;
    _pc_gdt[idx].limit_flags.raw = ((flags << 4) & 0xF0) | ((limit >> 16) & 0xF);

    _pc_gdt[idx].access_byte.raw = access;
}

void _pc_gdt_init(void)
{
    uintptr_t tss_base = (uintptr_t)&_pc_tss;

    memset(&_pc_gdt, 0, sizeof(_pc_gdt));
    
    set_gdt_entry(SEG_SEL_KERNEL_CODE >> 3, 0x00000000, 0xFFFFF, 0x9A, 0xC);
    set_gdt_entry(SEG_SEL_KERNEL_DATA >> 3, 0x00000000, 0xFFFFF, 0x92, 0xC);
    set_gdt_entry(SEG_SEL_USER_CODE >> 3, 0x00000000, 0xFFFFF, 0xFA, 0xC);
    set_gdt_entry(SEG_SEL_USER_DATA >> 3, 0x00000000, 0xFFFFF, 0xF2, 0xC);
    set_gdt_entry(SEG_SEL_TSS >> 3, tss_base, sizeof(_pc_tss) - 1, 0x89, 0x0);

    _pc_gdtr.size = sizeof(_pc_gdt) - 1;
    _pc_gdtr.gdt_ptr = (uint32_t)&_pc_gdt;

    _i686_lgdt(&_pc_gdtr);

    _pc_tss_init();

    gdt_load();

    _i686_ltr(SEG_SEL_TSS);
}
