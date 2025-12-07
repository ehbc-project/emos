#include <eboot/asm/bios/video.h>
#include <eboot/asm/bios/disk.h>
#include <eboot/asm/bios/bootinfo.h>

#include <eboot/compiler.h>

#include "../../filesystem/fat/fat.h"

static uint8_t sect_buf[4096] __aligned(16);
static uint8_t clus_buf[4096] __aligned(16);
static struct fat_bpb_sector *bpb = (struct fat_bpb_sector *)0x7C00;
static struct chs geom;

static int memcmp(const void *p1, const void *p2, size_t len)
{
    const uint8_t *a = p1, *b = p2;
    while (len--) {
        if (*a != *b) return (*a - *b);
        a++;
        b++;
    }
    return 0;
}

static status_t read_disk(lba_t lba, uint8_t count, void *buf)
{
    status_t status;
    struct chs chs;
    uint8_t result;

    lba += bpb->hidden_sector_count;

    chs.cylinder = lba / (geom.head * geom.sector);
    chs.head = (lba / geom.sector) % geom.head;
    chs.sector = (lba % geom.sector) + 1;

    status = _pc_bios_disk_read(_pc_boot_drive, chs, count, buf, &result);

    if (result != count) {
        return STATUS_UNEXPECTED_RESULT;
    }

    return STATUS_SUCCESS;
}

static void print_str(const char *str)
{
    while (*str) {
        _pc_bios_video_write_tty(*str++);
    }
}

void s1main(void)
{
    status_t status;

    status = _pc_bios_disk_get_params(_pc_boot_drive, NULL, NULL, &geom, NULL);
    if (!CHECK_SUCCESS(status)) {
        print_str("failed to request disk geometry");
        return;
    }

    uint16_t fat_lba = bpb->reserved_sector_count;
    uint32_t rootdir_lba = fat_lba + bpb->fat_size16 * bpb->fat_count;
    uint32_t cluster_start_lba = rootdir_lba + (bpb->root_entry_count * 32 + 511) / 512;
    uint16_t bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;

    union fat_dir_entry *entry;
    for (lba_t current_lba = rootdir_lba; current_lba < cluster_start_lba; current_lba++) {
        status = read_disk(current_lba, 1, sect_buf);
        if (!CHECK_SUCCESS(status)) {
            print_str("failed to read disk");
            for (;;) {}
        }
    
        for (int i = 0; i < 32; i++) {
            entry = &((union fat_dir_entry *)sect_buf)[i];
            
            if (memcmp(entry->file.name_ext, "BOOTLDR X86", sizeof(entry->file.name) + sizeof(entry->file.extension)) == 0) {
                goto file_found;
            } else if (!entry->file.name[0]) {
                print_str("BOOTLDR.X86 not found\r\n");
                return;
            }
        }
    }

file_found: {}
    uint16_t current_cluster = entry->file.cluster_location;

    uint32_t *load_dest = (uint32_t *)0x00100000;

    status = read_disk(fat_lba, 8, sect_buf);
    if (!CHECK_SUCCESS(status)) {
        print_str("failed to read disk");
        for (;;) {}
    }
    while (current_cluster <= FAT12_MAX_CLUSTER) {
        read_disk(cluster_start_lba + (current_cluster - 2) * bpb->sectors_per_cluster, bpb->sectors_per_cluster, clus_buf);
        if (!CHECK_SUCCESS(status)) {
            print_str("failed to read disk");
            for (;;) {}
        }

        for (int i = 0; i < bytes_per_cluster / sizeof(uint32_t); i++) {
            *load_dest++ = ((uint32_t *)clus_buf)[i];
        }
        _pc_bios_video_write_tty('.');

        if (current_cluster & 1) {
            current_cluster = (sect_buf[current_cluster * 3 / 2 + 1] << 4) | (sect_buf[current_cluster * 3 / 2] >> 4);
        } else {
            current_cluster = ((sect_buf[current_cluster * 3 / 2 + 1] & 0xF) << 8) | sect_buf[current_cluster * 3 / 2];
        }
    }
    print_str("\r\n");

    ((void (*)(void))0x00100000)();

    return;
}
