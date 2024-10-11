/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to initialize all devices,
    since the kernel will do the rest.
*/

#include <stdint.h>

union data_reg {
    uint32_t l;
    uint16_t w;
    struct {
        uint8_t l, h;
    } b;
};

union index_reg {
    uint32_t l;
    uint16_t w;
    uint8_t b;
};

struct regs {
    union data_reg a;
    union data_reg b;
    union data_reg c;
    union data_reg d;
    union index_reg si;
    union index_reg di;
    union index_reg es;
};

extern int bios_call(uint8_t irq, struct regs*);

void main(void)
{
    const char str[] = "Hello, World!\r\n";
    struct regs irq_regs = {
        .a.b.h = 0x0E,
        .a.b.l = 'H',
    };
    for (int i = 0; str[i]; i++) {
        irq_regs.a.b.l = str[i];
        bios_call(0x10, &irq_regs);
    }
    return;
}
