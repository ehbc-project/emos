#include "asm/bootinfo.h"

#include <stdint.h>

__attribute__((section(".data.low")))
uint8_t _pc_boot_drive = 0xFF;

