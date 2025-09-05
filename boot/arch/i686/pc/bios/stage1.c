#include <asm/bios/video.h>
#include <asm/bios/disk.h>
#include <asm/bootinfo.h>

#include "../../fs/fat/fat.h"

static uint8_t sect_buf[4096] __attribute__((aligned(16)));
static uint8_t clus_buf[4096] __attribute__((aligned(16)));
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

static int read_disk(lba_t lba, uint16_t count, void *buf)
{
    struct chs chs;
    lba += bpb->hidden_sector_count;
    chs.cylinder = lba / (geom.head * geom.sector);
    chs.head = (lba / geom.sector) % geom.head;
    chs.sector = (lba % geom.sector) + 1;
    return _pc_bios_read_drive(_pc_boot_drive, chs, count, buf);
}

static void print_str(const char *str)
{
    while (*str) {
        _pc_bios_tty_output(*str++);
    }
}

__attribute__((noreturn))
void s1main(void)
{
    _pc_bios_get_drive_params(_pc_boot_drive, NULL, &geom, NULL);

    uint16_t fat_lba = bpb->reserved_sector_count;
    uint32_t rootdir_lba = fat_lba + bpb->fat_size16 * bpb->fat_count;
    uint32_t cluster_start_lba = rootdir_lba + (bpb->root_entry_count * 32 + 511) / 512;
    uint16_t bytes_per_cluster = bpb->bytes_per_sector * bpb->sectors_per_cluster;

    union fat_dir_entry *entry;
    for (lba_t current_lba = rootdir_lba; current_lba < cluster_start_lba; current_lba++) {
        read_disk(current_lba, 1, sect_buf);
        
        for (int i = 0; i < 32; i++) {
            entry = &((union fat_dir_entry *)sect_buf)[i];
            
            if (memcmp(entry->file.name, "BOOTLDR X86", sizeof(entry->file.name) + sizeof(entry->file.extension)) == 0) {
                goto file_found;
            } else if (!entry->file.name[0]) {
                goto file_not_found;
            }
        }
    }
file_not_found:
    print_str("BOOTLDR.X86 not found\r\n");
    for (;;) {}

file_found: {}
    uint16_t current_cluster = entry->file.cluster_location;

    uint32_t *load_dest = (uint32_t *)0x00100000;

    read_disk(fat_lba, 8, sect_buf);
    while (current_cluster <= FAT12_MAX_CLUSTER) {
        read_disk(cluster_start_lba + (current_cluster - 2) * bpb->sectors_per_cluster, bpb->sectors_per_cluster, clus_buf);
        for (int i = 0; i < bytes_per_cluster / sizeof(uint32_t); i++) {
            *load_dest++ = ((uint32_t *)clus_buf)[i];
        }
        _pc_bios_tty_output('.');

        if (current_cluster & 1) {
            current_cluster = (sect_buf[current_cluster * 3 / 2 + 1] << 4) | (sect_buf[current_cluster * 3 / 2] >> 4);
        } else {
            current_cluster = ((sect_buf[current_cluster * 3 / 2 + 1] & 0xF) << 8) | sect_buf[current_cluster * 3 / 2];
        }
    }
    print_str("\r\n");

    ((void (*)(void))0x00100000)();

    for (;;) {}
}
