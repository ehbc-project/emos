#ifndef __I686_PC_BIOS_DISK_H__
#define __I686_PC_BIOS_DISK_H__

#include <stdint.h>

#include <asm/farptr.h>
#include <disk/disk.h>

enum bios_drive_type {
    BIOS_DRIVE_360K = 0,
    BIOS_DRIVE_1_2M,
    BIOS_DRIVE_720k,
    BIOS_DRIVE_1_44M,
    BIOS_DRIVE_2_88M_ALT,
    BIOS_DRIVE_2_88M,
    BIOS_DRIVE_ATAPI = 0x10,
};

struct bios_disk_base_table {
    uint8_t data[11];
};

enum bios_edd_version {
    BIOS_EDD_1_X = 0x01,
    BIOS_EDD_2_0 = 0x20,
    BIOS_EDD_2_1 = 0x21,
    BIOS_EDD_3_0 = 0x30,
};

struct bios_extended_drive_params {
    uint16_t table_size;
    uint16_t flags;
    uint32_t geom_cylinders;
    uint32_t geom_heads;
    uint32_t geom_sectors;
    uint64_t total_sectors;
    uint16_t bytes_per_sector;

    struct farptr edd_config_params;

    uint16_t device_path_signature;
    uint8_t device_path_size;
    uint8_t reserved1[3];
    char host_bus[4];
    char interface[8];
    char interface_path[8];
    char device_path[8];
    uint8_t reserved2;
    uint8_t device_path_checksum;
};

int _pc_bios_reset_drive(uint8_t drive);
int _pc_bios_read_drive(uint8_t drive, struct chs chs, uint8_t count, void *buf);
int _pc_bios_write_drive(uint8_t drive, struct chs chs, uint8_t count, const void *buf);
int _pc_bios_get_drive_params(uint8_t drive, enum bios_drive_type *type, struct chs *geometry, struct bios_disk_base_table **dbt);
int _pc_bios_check_ext(uint8_t drive, enum bios_edd_version *edd_version, uint16_t *subset_support_flags);
int _pc_bios_read_drive_ext(uint8_t drive, lba_t lba, uint16_t count, void *buf);
int _pc_bios_write_drive_ext(uint8_t drive, lba_t lba, uint16_t count, const void *buf);
int _pc_bios_get_drive_params_ext(uint8_t drive, struct bios_extended_drive_params *params);

#endif // __I686_PC_BIOS_DISK_H__
