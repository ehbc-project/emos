#ifndef __EBOOT_ASM_BIOS_BOOTINFO_H__
#define __EBOOT_ASM_BIOS_BOOTINFO_H__

#include <stdint.h>

extern const uint8_t _pc_boot_drive;
extern uint32_t _pc_boot_part_base;
extern uint8_t _pc_boot_sector[512];

#endif // __EBOOT_ASM_BIOS_BOOTINFO_H__
