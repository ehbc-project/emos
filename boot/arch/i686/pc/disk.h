#ifndef __I686_PC_DISK_H__
#define __I686_PC_DISK_H__

#include <stdint.h>

#include "../../../common/disk.h"

int _pc_bios_reset_drive(uint8_t drive);
int _pc_bios_read_drive_chs(uint8_t drive, struct chs chs, uint8_t count, void *buf);

#endif // __I686_PC_DISK_H__
