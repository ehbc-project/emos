#ifndef __I686_PC_BIOS_DISK_H__
#define __I686_PC_BIOS_DISK_H__

#include <stdint.h>

#include "disk/disk.h"

int _pc_bios_reset_drive(uint8_t drive);
int _pc_bios_read_drive_chs(uint8_t drive, struct chs chs, uint8_t count, void *buf);
int _pc_bios_read_drive_lba(uint8_t drive, lba_t lba, uint16_t count, void *buf);

#endif // __I686_PC_BIOS_DISK_H__
