#include <asm/isr.h>

#include <stdio.h>
#include <sys/io.h>
#include <asm/idt.h>
#include <mm/mm.h>
#include <debug.h>
#include <macros.h>

#define DECLARE_ISRxy(x, y) extern void _pc_isr_##x##y(void);
#define DECLARE_ISRx(x) \
    DECLARE_ISRxy(x, 0) DECLARE_ISRxy(x, 1) \
    DECLARE_ISRxy(x, 2) DECLARE_ISRxy(x, 3) \
    DECLARE_ISRxy(x, 4) DECLARE_ISRxy(x, 5) \
    DECLARE_ISRxy(x, 6) DECLARE_ISRxy(x, 7) \
    DECLARE_ISRxy(x, 8) DECLARE_ISRxy(x, 9) \
    DECLARE_ISRxy(x, a) DECLARE_ISRxy(x, b) \
    DECLARE_ISRxy(x, c) DECLARE_ISRxy(x, d) \
    DECLARE_ISRxy(x, e) DECLARE_ISRxy(x, f)

DECLARE_ISRx(0) DECLARE_ISRx(1) DECLARE_ISRx(2) DECLARE_ISRx(3)
DECLARE_ISRx(4) DECLARE_ISRx(5) DECLARE_ISRx(6) DECLARE_ISRx(7)
DECLARE_ISRx(8) DECLARE_ISRx(9) DECLARE_ISRx(a) DECLARE_ISRx(b)
DECLARE_ISRx(c) DECLARE_ISRx(d) DECLARE_ISRx(e) DECLARE_ISRx(f)

#define SET_INT_ENTRY(num) \
    _pc_idt[0x##num] = (struct idt_entry){ \
        .offset_low = (uint32_t)_pc_isr_##num & 0xFFFF, \
        .segment_selector = 0x0008, \
        .attributes = 0xEE, \
        .offset_high = ((uint32_t)_pc_isr_##num >> 16) & 0xFFFF, \
    }

#define SET_TRAP_ENTRY(num) \
    _pc_idt[0x##num] = (struct idt_entry){ \
        .offset_low = (uint32_t)_pc_isr_##num & 0xFFFF, \
        .segment_selector = 0x0008, \
        .attributes = 0xEF, \
        .offset_high = ((uint32_t)_pc_isr_##num >> 16) & 0xFFFF, \
    }

extern struct idt_entry *_i686_idt_ptr;

struct idt_entry _pc_idt[256];
struct isr_table_entry *_pc_isr_table[256];

void _pc_init_idt(void)
{
    _i686_idt_ptr = _pc_idt;
    asm(
        "pushl  $%0 \n"
        "pushw  $2047\n"
        "lidt   (%%esp)\n"
        "add    $6, %%esp\n"
        :
        : "m"(_pc_idt)
        :
    );

    for (int i = 0; i < ARRAY_SIZE(_pc_isr_table); i++) {
        _pc_isr_table[i] = NULL;
    }

    SET_INT_ENTRY(00);  /* #DE */
    SET_TRAP_ENTRY(01);  /* #DB */
    SET_INT_ENTRY(02);  /* NMI */
    SET_TRAP_ENTRY(03);  /* #BP */
    SET_TRAP_ENTRY(04);  /* #OF */
    SET_INT_ENTRY(05);  /* #BR */
    SET_INT_ENTRY(06);  /* #UD */
    SET_INT_ENTRY(07);  /* #NM */
    SET_INT_ENTRY(08);  /* #DF */
    SET_INT_ENTRY(09);
    SET_INT_ENTRY(0a);  /* #TS */
    SET_INT_ENTRY(0b);  /* #NP */
    SET_INT_ENTRY(0c);  /* #SS */
    SET_INT_ENTRY(0d);  /* #GP */
    SET_INT_ENTRY(0e);  /* #PF */
    SET_INT_ENTRY(0f);
    SET_INT_ENTRY(10);  /* #MF */
    SET_INT_ENTRY(11);  /* #AC */
    SET_INT_ENTRY(12);  /* #MC */
    SET_INT_ENTRY(13);  /* #XM */
    SET_INT_ENTRY(14);  /* #VE */
    SET_INT_ENTRY(15);  /* #CP */
    SET_INT_ENTRY(16);
    SET_INT_ENTRY(17);
    SET_INT_ENTRY(18);
    SET_INT_ENTRY(19);
    SET_INT_ENTRY(1a);
    SET_INT_ENTRY(1b);
    SET_INT_ENTRY(1c);  /* #HV */
    SET_INT_ENTRY(1d);  /* #VC */
    SET_INT_ENTRY(1e);  /* #SX */
    SET_INT_ENTRY(1f);
    
    /* hardware interrupts (PIC) */
    SET_INT_ENTRY(20); SET_INT_ENTRY(21); SET_INT_ENTRY(22); SET_INT_ENTRY(23);
    SET_INT_ENTRY(24); SET_INT_ENTRY(25); SET_INT_ENTRY(26); SET_INT_ENTRY(27);
    SET_INT_ENTRY(28); SET_INT_ENTRY(29); SET_INT_ENTRY(2a); SET_INT_ENTRY(2b);
    SET_INT_ENTRY(2c); SET_INT_ENTRY(2d); SET_INT_ENTRY(2e); SET_INT_ENTRY(2f);

    /* hardware interrupts */
    SET_INT_ENTRY(30); SET_INT_ENTRY(31); SET_INT_ENTRY(32); SET_INT_ENTRY(33);
    SET_INT_ENTRY(34); SET_INT_ENTRY(35); SET_INT_ENTRY(36); SET_INT_ENTRY(37);
    SET_INT_ENTRY(38); SET_INT_ENTRY(39); SET_INT_ENTRY(3a); SET_INT_ENTRY(3b);
    SET_INT_ENTRY(3c); SET_INT_ENTRY(3d); SET_INT_ENTRY(3e); SET_INT_ENTRY(3f);
    SET_INT_ENTRY(40); SET_INT_ENTRY(41); SET_INT_ENTRY(42); SET_INT_ENTRY(43);
    SET_INT_ENTRY(44); SET_INT_ENTRY(45); SET_INT_ENTRY(46); SET_INT_ENTRY(47);
    SET_INT_ENTRY(48); SET_INT_ENTRY(49); SET_INT_ENTRY(4a); SET_INT_ENTRY(4b);
    SET_INT_ENTRY(4c); SET_INT_ENTRY(4d); SET_INT_ENTRY(4e); SET_INT_ENTRY(4f);
    SET_INT_ENTRY(50); SET_INT_ENTRY(51); SET_INT_ENTRY(52); SET_INT_ENTRY(53);
    SET_INT_ENTRY(54); SET_INT_ENTRY(55); SET_INT_ENTRY(56); SET_INT_ENTRY(57);
    SET_INT_ENTRY(58); SET_INT_ENTRY(59); SET_INT_ENTRY(5a); SET_INT_ENTRY(5b);
    SET_INT_ENTRY(5c); SET_INT_ENTRY(5d); SET_INT_ENTRY(5e); SET_INT_ENTRY(5f);
    SET_INT_ENTRY(60); SET_INT_ENTRY(61); SET_INT_ENTRY(62); SET_INT_ENTRY(63);
    SET_INT_ENTRY(64); SET_INT_ENTRY(65); SET_INT_ENTRY(66); SET_INT_ENTRY(67);
    SET_INT_ENTRY(68); SET_INT_ENTRY(69); SET_INT_ENTRY(6a); SET_INT_ENTRY(6b);
    SET_INT_ENTRY(6c); SET_INT_ENTRY(6d); SET_INT_ENTRY(6e); SET_INT_ENTRY(6f);
    SET_INT_ENTRY(70); SET_INT_ENTRY(71); SET_INT_ENTRY(72); SET_INT_ENTRY(73);
    SET_INT_ENTRY(74); SET_INT_ENTRY(75); SET_INT_ENTRY(76); SET_INT_ENTRY(77);
    SET_INT_ENTRY(78); SET_INT_ENTRY(79); SET_INT_ENTRY(7a); SET_INT_ENTRY(7b);
    SET_INT_ENTRY(7c); SET_INT_ENTRY(7d); SET_INT_ENTRY(7e); SET_INT_ENTRY(7f);

    /* traps */
    SET_TRAP_ENTRY(80); SET_TRAP_ENTRY(81); SET_TRAP_ENTRY(82); SET_TRAP_ENTRY(83);
    SET_TRAP_ENTRY(84); SET_TRAP_ENTRY(85); SET_TRAP_ENTRY(86); SET_TRAP_ENTRY(87);
    SET_TRAP_ENTRY(88); SET_TRAP_ENTRY(89); SET_TRAP_ENTRY(8a); SET_TRAP_ENTRY(8b);
    SET_TRAP_ENTRY(8c); SET_TRAP_ENTRY(8d); SET_TRAP_ENTRY(8e); SET_TRAP_ENTRY(8f);
    SET_TRAP_ENTRY(90); SET_TRAP_ENTRY(91); SET_TRAP_ENTRY(92); SET_TRAP_ENTRY(93);
    SET_TRAP_ENTRY(94); SET_TRAP_ENTRY(95); SET_TRAP_ENTRY(96); SET_TRAP_ENTRY(97);
    SET_TRAP_ENTRY(98); SET_TRAP_ENTRY(99); SET_TRAP_ENTRY(9a); SET_TRAP_ENTRY(9b);
    SET_TRAP_ENTRY(9c); SET_TRAP_ENTRY(9d); SET_TRAP_ENTRY(9e); SET_TRAP_ENTRY(9f);
    SET_TRAP_ENTRY(a0); SET_TRAP_ENTRY(a1); SET_TRAP_ENTRY(a2); SET_TRAP_ENTRY(a3);
    SET_TRAP_ENTRY(a4); SET_TRAP_ENTRY(a5); SET_TRAP_ENTRY(a6); SET_TRAP_ENTRY(a7);
    SET_TRAP_ENTRY(a8); SET_TRAP_ENTRY(a9); SET_TRAP_ENTRY(aa); SET_TRAP_ENTRY(ab);
    SET_TRAP_ENTRY(ac); SET_TRAP_ENTRY(ad); SET_TRAP_ENTRY(ae); SET_TRAP_ENTRY(af);
    SET_TRAP_ENTRY(b0); SET_TRAP_ENTRY(b1); SET_TRAP_ENTRY(b2); SET_TRAP_ENTRY(b3);
    SET_TRAP_ENTRY(b4); SET_TRAP_ENTRY(b5); SET_TRAP_ENTRY(b6); SET_TRAP_ENTRY(b7);
    SET_TRAP_ENTRY(b8); SET_TRAP_ENTRY(b9); SET_TRAP_ENTRY(ba); SET_TRAP_ENTRY(bb);
    SET_TRAP_ENTRY(bc); SET_TRAP_ENTRY(bd); SET_TRAP_ENTRY(be); SET_TRAP_ENTRY(bf);
    SET_TRAP_ENTRY(c0); SET_TRAP_ENTRY(c1); SET_TRAP_ENTRY(c2); SET_TRAP_ENTRY(c3);
    SET_TRAP_ENTRY(c4); SET_TRAP_ENTRY(c5); SET_TRAP_ENTRY(c6); SET_TRAP_ENTRY(c7);
    SET_TRAP_ENTRY(c8); SET_TRAP_ENTRY(c9); SET_TRAP_ENTRY(ca); SET_TRAP_ENTRY(cb);
    SET_TRAP_ENTRY(cc); SET_TRAP_ENTRY(cd); SET_TRAP_ENTRY(ce); SET_TRAP_ENTRY(cf);
    SET_TRAP_ENTRY(d0); SET_TRAP_ENTRY(d1); SET_TRAP_ENTRY(d2); SET_TRAP_ENTRY(d3);
    SET_TRAP_ENTRY(d4); SET_TRAP_ENTRY(d5); SET_TRAP_ENTRY(d6); SET_TRAP_ENTRY(d7);
    SET_TRAP_ENTRY(d8); SET_TRAP_ENTRY(d9); SET_TRAP_ENTRY(da); SET_TRAP_ENTRY(db);
    SET_TRAP_ENTRY(dc); SET_TRAP_ENTRY(dd); SET_TRAP_ENTRY(de); SET_TRAP_ENTRY(df);
    SET_TRAP_ENTRY(e0); SET_TRAP_ENTRY(e1); SET_TRAP_ENTRY(e2); SET_TRAP_ENTRY(e3);
    SET_TRAP_ENTRY(e4); SET_TRAP_ENTRY(e5); SET_TRAP_ENTRY(e6); SET_TRAP_ENTRY(e7);
    SET_TRAP_ENTRY(e8); SET_TRAP_ENTRY(e9); SET_TRAP_ENTRY(ea); SET_TRAP_ENTRY(eb);
    SET_TRAP_ENTRY(ec); SET_TRAP_ENTRY(ed); SET_TRAP_ENTRY(ee); SET_TRAP_ENTRY(ef);
    SET_TRAP_ENTRY(f0); SET_TRAP_ENTRY(f1); SET_TRAP_ENTRY(f2); SET_TRAP_ENTRY(f3);
    SET_TRAP_ENTRY(f4); SET_TRAP_ENTRY(f5); SET_TRAP_ENTRY(f6); SET_TRAP_ENTRY(f7);
    SET_TRAP_ENTRY(f8); SET_TRAP_ENTRY(f9); SET_TRAP_ENTRY(fa); SET_TRAP_ENTRY(fb);
    SET_TRAP_ENTRY(fc); SET_TRAP_ENTRY(fd); SET_TRAP_ENTRY(fe); SET_TRAP_ENTRY(ff);
}

void _pc_set_interrupt_handler(int num, struct device *dev, interrupt_handler_t func)
{
    struct isr_table_entry *entry;
    if (!_pc_isr_table[num]) {
        _pc_isr_table[num] = mm_allocate(sizeof(**_pc_isr_table));
        entry = _pc_isr_table[num];
    } else {
        entry = _pc_isr_table[num];
        while (entry) {
            entry = entry->next;
        }
    }
    entry->next = NULL;
    entry->dev = dev;
    entry->interrupt_handler = func;
    entry->is_interrupt = 1;
}

void _pc_set_trap_handler(int num, trap_handler_t func)
{
    struct isr_table_entry *entry;
    if (!_pc_isr_table[num]) {
        _pc_isr_table[num] = mm_allocate(sizeof(**_pc_isr_table));
        entry = _pc_isr_table[num];
    } else {
        entry = _pc_isr_table[num];
        while (entry) {
            entry = entry->next;
        }
    }
    entry->next = NULL;
    entry->dev = NULL;
    entry->trap_handler = func;
    entry->is_interrupt = 0;
}

struct isr_table_entry *_pc_get_isr_table_entry(int num)
{
    return _pc_isr_table[num];
}

static uint64_t irq_count = 0;

uint64_t _pc_get_irq_count(void)
{
    return irq_count;
}

void _pc_isr_common(struct interrupt_frame *frame, struct trap_regs *regs, int num, int error)
{
    int has_error = 0, is_fault = 0;
    irq_count++;

    if (num < 0x20) {
        switch (num) {
            case 0x08:
            case 0x0A:
            case 0x0B:
            case 0x0C:
            case 0x0D:
            case 0x0E:
            case 0x11:
            case 0x15:
            case 0x1D:
            case 0x1E:
                has_error = 1;
            case 0x00:
            case 0x05:
            case 0x06:
            case 0x07:
            case 0x09:
            case 0x10:
            case 0x13:
            case 0x14:
                is_fault = 1;
                break;
        }
    } else {
        if (num < 0x30) {
            if (num >= 0x28) {
                io_out8(0xA0, 0x20);
            }
            io_out8(0x20, 0x20);
        }
    }

    if (is_fault) {
        if (has_error) {
            fprintf(stderr, "\nFault/Abort #%02X (0x%08X) at 0x%04X:0x%08lX", num, error, frame->cs, frame->eip);
        } else {
            fprintf(stderr, "\nFault/Abort #%02X at 0x%04X:0x%08lX", num, frame->cs, frame->eip);
        }
        panic("Unrecoverable fault");
        return;
    }

    struct isr_table_entry *current_isr = _pc_isr_table[num];
    if (!current_isr) {
        if (has_error) {
            fprintf(stderr, "Unhandled interrupt #%02X (error 0x%08X) at 0x%04X:0x%08lX\n", num, error, frame->cs, frame->eip);
        } else if (num != 0x20 && num != 0x21 && num != 0x28 && num != 0x2C) {
            fprintf(stderr, "Unhandled interrupt #%02X at 0x%04X:0x%08lX\n", num, frame->cs, frame->eip);
        }
    }

    while (current_isr) {
        if (current_isr->is_interrupt && current_isr->interrupt_handler) {
            current_isr->interrupt_handler(current_isr->dev, num);
        } else if (current_isr->trap_handler) {
            current_isr->trap_handler(frame, regs, num, error);
        }
        current_isr = current_isr->next;
    }
}
