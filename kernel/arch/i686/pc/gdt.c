#include <emos/asm/gdt.h>

#include <string.h>

#include <emos/compiler.h>

struct gdt_entry _pc_gdt[5];
static struct gdtr _pc_gdtr;

static void gdt_load(void) {
    
    asm volatile(
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "ljmp $0x08, $1f\n\t"
        "1:\n"
        : : "r" (&_pc_gdtr) : "memory", "ax"
    );
}

void _pc_gdt_init(void)
{
    _pc_gdtr.size = sizeof(_pc_gdt) - 1;
    _pc_gdtr.gdt_ptr = (uint32_t)&_pc_gdt;

    _pc_gdt[1].limit_low = 0xFFFF;
    _pc_gdt[1].base_low = 0x0000;
    _pc_gdt[1].base_mid = 0x00;
    _pc_gdt[1].base_high = 0x00;
    _pc_gdt[1].access_byte.raw = 0x9A;
    _pc_gdt[1].limit_flags.raw = 0xCF;

    _pc_gdt[2].limit_low = 0xFFFF;
    _pc_gdt[2].base_low = 0x0000;
    _pc_gdt[2].base_mid = 0x00;
    _pc_gdt[2].base_high = 0x00;
    _pc_gdt[2].access_byte.raw = 0x92;
    _pc_gdt[2].limit_flags.raw = 0xCF;

    gdt_load();
}
