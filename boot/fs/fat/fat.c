#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <ctype.h>

#include <compiler.h>
#include <mm/mm.h>
#include <device/driver.h>
#include <fs/driver.h>
#include <interface/block.h>
#include <disk/disk.h>

#include <sys/io.h>

#include "fat.h"

struct fat_dir_data {
    fatcluster_t head_cluster, current_cluster;
    unsigned int current_entry_index;
    struct fat_direntry_file direntry;
};

struct fat_file_data {
    fatcluster_t direntry_cluster;
    unsigned int direntry_entry_index;
    struct fat_direntry_file direntry;

    fatcluster_t head_cluster, current_cluster;
    uint32_t cursor;
};

struct fat_data {
    struct device *blkdev;
    const struct block_interface *blkif;

    uint16_t    reserved_sectors;
    uint16_t    sector_size;
    uint32_t    cluster_size;
    uint8_t     sectors_per_cluster;
    uint8_t     fat_type;
    uint32_t    data_area_begin;
    uint32_t    fat_size;
    uint32_t    free_clusters;
    uint32_t    next_free_cluster;
    uint32_t    total_sector_count;
    uint32_t    root_cluster;
    uint16_t    root_entry_count;
    uint16_t    root_sector_count;

    lba_t       fatbuf_lba;
    uint8_t     fatbuf[FAT_SECTOR_SIZE];
    lba_t       databuf_lba_start;
    uint8_t     *databuf;

    int         (*sector_to_cluster)(struct filesystem *, fatcluster_t *, lba_t);
    int         (*cluster_to_sector)(struct filesystem *, lba_t *, fatcluster_t);
    int         (*get_next_cluster)(struct filesystem *, fatcluster_t *, unsigned int);
};

static size_t
remove_right_padding(
    char* dest,
    const char* orig,
    size_t dest_len,
    size_t orig_len)
{
    int str_len = 0;
    for (int cur = orig_len - 1; cur >= 0; cur--) {
        if (str_len) {
            dest[cur] = orig[cur];
            str_len++;
        } else if (orig[cur] != ' ') {
            str_len++;
            dest[cur] = orig[cur];
            if (cur < dest_len) {
                dest[cur + 1] = 0;
            }
        }
    }
    return str_len;
}

static int validate_sfn(const char* str, size_t len)
{
    /*  Characters Allowed:
        - A-Z
        - 0-9
        - char > 127
        - (space) $ % - _ @ ~ ` ! ( ) { } ^ # &

        Invalid Names:
        - .
        - ..

        Reference: https://averstak.tripod.com/fatdox/names.htm
     */
    static const uint32_t bitmap[] = {
        0x00000000, 0x03FF237B, /* ASCII 0x00 - 0x3F */
        0xC3FFFFFF, 0x68000001, /* ASCII 0x40 - 0x7F */
    };

    int has_dot = 0;

    if (len > 0 && str[0] == '.') {
        return 0;
    }
    
    for (int i = 0; str[i] != 0 && i < len; i++) {
        if (str[i] > 0x7F) {
            continue;
        } else if (str[i] == '.') {
            if (has_dot) {
                return 0;
            }
            has_dot = 1;
        }
        const uint32_t bmval = bitmap[str[i] >> 5];
        if (!((bmval >> (str[i] & 31)) & 1)) {
            return 0;
        }
    }
    return 1;
}

static int validate_lfn(const char* str, size_t len)
{
    /*  Characters Not Allowed:
        - \ / : * ? " < > |
        - Control characters

        Invalid Names:
        - .
        - ..

        Reference: https://en.wikipedia.org/wiki/Long_filename
     */
    static const uint32_t bitmap[] = {
        0x00000000, 0x23FF7BFB, /* ASCII 0x00 - 0x3F */
        0xFFFFFFFF, 0x6FFFFFFF, /* ASCII 0x40 - 0x7F */
    };

    if (len > 0 && str[0] == '.') {
        return 0;
    }
    
    for (int i = 0; str[i] != 0 && i < len; i++) {
        if (i > 0x7F) {
            continue;
        }
        const uint32_t bmval = bitmap[str[i] >> 5];
        if (!((bmval >> (str[i] & 31)) & 1)) {
            return 0;
        }
    }
    return 1;
}

static int ucs2_to_utf8(char* buf, int len, uint16_t ucs2ch)
{
    if (ucs2ch == 0 || ucs2ch == 0xFFFF) return 0;

    if (ucs2ch < 0x7F) {
        if (len < 1) return -1;
        *buf = ucs2ch;
        return 1;
    }

    if (ucs2ch < 0x7FF) {
        if (len < 2) return -1;
        *buf++ = ((ucs2ch & 0x07C0) >> 6) | 0xC0;
        *buf++ = (ucs2ch & 0x003F) | 0x80;
        return 2;
    }

    if (len < 3) return -1;
    *buf++ = ((ucs2ch & 0xF000) >> 12) | 0xE0;
    *buf++ = ((ucs2ch & 0x0FC0) >> 6) | 0x80;
    *buf++ = (ucs2ch & 0x003F) | 0x80;
    return 3;
}

static size_t
get_lfn_filename(
    const struct fat_direntry_lfn* entry,
    uint16_t buf[static FAT_LFN_BUFLEN])
{
    buf += ((entry->sequence_index & 0x1F) - 1) * 13;
    size_t char_count = 0;
    for (int i = 0; i < 5; i++) {
        buf[char_count++] = entry->name_fragment1[i];
    }
    for (int i = 0; i < 6; i++) {
        buf[char_count++] = entry->name_fragment2[i];
    }
    for (int i = 0; i < 2; i++) {
        buf[char_count++] = entry->name_fragment3[i];
    }
    if (entry->sequence_index & 0x40) {
        buf[char_count] = '\0';
    }
    return char_count;
}

static size_t
lfn_ucs2_to_utf8(
    char utf8buf[static FAT_LFN_U8_BUFLEN],
    const uint16_t ucs2buf[static FAT_LFN_BUFLEN],
    int allow_nonascii,
    char fallback)
{
    size_t bytes_written = 0;
    if (allow_nonascii) {
        size_t buflen = FAT_LFN_U8_BUFLEN;
        while (bytes_written < FAT_LFN_U8_BUFLEN) {
            int u8ch_len = ucs2_to_utf8(utf8buf, buflen, *ucs2buf++);
            if (!u8ch_len) {
                *utf8buf = 0;
                break;
            } else if (u8ch_len < 0) {
                *utf8buf = fallback;
                u8ch_len = 1;
            }
            utf8buf += u8ch_len;
            bytes_written += u8ch_len;
            buflen -= u8ch_len;
        }
    } else {
        while (bytes_written < FAT_LFN_BUFLEN) {
            *utf8buf++ = *ucs2buf < 0x80 ? *ucs2buf : fallback;
            ucs2buf++;
            bytes_written++;
        }
        *utf8buf = 0;
    }

    return bytes_written;
}

static size_t
get_sfn_filename(
    const struct fat_direntry_file* entry,
    char *buf)
{
    size_t char_count = 0;
    for (int i = 0; i < 8 && entry->name[i] != ' '; i++) {
        buf[char_count++] =
            (entry->attribute2 & FAT_ATTR2_LCASE_NAME) ? tolower(entry->name[i]) : entry->name[i];
    }
    if (entry->extension[0] != ' ') {
        buf[char_count++] = '.';
    }
    for (int i = 0; i < 3 && entry->extension[i] != ' '; i++) {
        buf[char_count++] =
            (entry->attribute2 & FAT_ATTR2_LCASE_EXT) ? tolower(entry->extension[i]) : entry->extension[i];
    }
    buf[char_count] = '\0';

    return char_count;
}

static uint8_t get_sfn_checksum(char buf[static FAT_SFN_BUFLEN])
{
    uint8_t chksum = 0;
    for (int i = FAT_SFN_BUFLEN; i != 0; i--) {
        chksum = ((chksum & 1) ? 0x80 : 0) + (chksum >> 1) + *buf++;
    }

    return chksum;
}

static int read_fat(struct filesystem *fs, uint32_t fat_sector)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    lba_t lba = data->reserved_sectors + fat_sector;
    if (data->fatbuf_lba == lba) return 0;
    
    data->fatbuf_lba = lba;
    return data->blkif->read(data->blkdev, lba, data->fatbuf, 1);
}

static int sector_to_cluster12_16(struct filesystem *fs, fatcluster_t *cluster, lba_t lba)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    *cluster = (lba - data->data_area_begin - data->root_sector_count) / data->sectors_per_cluster + 2;
    return 0;
}

static int sector_to_cluster32(struct filesystem *fs, fatcluster_t *cluster, lba_t lba)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    *cluster = (lba - data->data_area_begin) / data->sectors_per_cluster + 2;
    return 0;
}

static int sector_to_cluster(struct filesystem *fs, fatcluster_t *cluster, lba_t lba)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    return data->sector_to_cluster(fs, cluster, lba);
}

static int cluster_to_sector12_16(struct filesystem *fs, lba_t *lba, fatcluster_t cluster)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    *lba = ((cluster - 2) * data->sectors_per_cluster) + data->data_area_begin + data->root_sector_count;
    return 0;
}

static int cluster_to_sector32(struct filesystem *fs, lba_t *lba, fatcluster_t cluster)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    *lba = ((cluster - 2) * data->sectors_per_cluster) + data->data_area_begin;
    return 0;
}

static int cluster_to_sector(struct filesystem *fs, lba_t *lba, fatcluster_t cluster)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    return data->cluster_to_sector(fs, lba, cluster);
}

static int read_sector(struct filesystem* fs, lba_t lba)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    if (data->databuf_lba_start == lba) return 0;
    
    data->databuf_lba_start = lba;
    return data->blkif->read(data->blkdev, lba, data->databuf, 1);
}

static int read_cluster(struct filesystem* fs, fatcluster_t cluster)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    lba_t lba;
    cluster_to_sector(fs, &lba, cluster);
    if (data->databuf_lba_start == lba) return 0;

    data->databuf_lba_start = lba;
    return data->blkif->read(data->blkdev, lba, data->databuf, data->sectors_per_cluster);
}

static int get_next_cluster12(struct filesystem *fs, fatcluster_t *cluster, unsigned int count)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    if (*cluster > FAT12_MAX_CLUSTER) {
        return 1;
    }

    while (count-- > 0) {
        uint16_t byte_idx = *cluster + (*cluster >> 1);
        uint16_t sector_idx = byte_idx / data->sector_size;
        byte_idx %= data->sector_size;

        read_fat(fs, sector_idx);

        uint8_t fatentry_buf[2];

        fatentry_buf[0] = data->fatbuf[byte_idx];
        if (byte_idx == data->sector_size - 1) {
            unsigned int entry_idx;
            read_fat(fs, sector_idx + 1);
            fatentry_buf[1] = data->fatbuf[0];
        } else {
            fatentry_buf[1] = data->fatbuf[byte_idx + 1];
        }

        if (*cluster & 1) {  /* odd-numbered cluster */
            *cluster = 
                ((fatentry_buf[0] & 0xF0) >> 4) |
                (fatentry_buf[1] << 4);
        } else {  /* even-numbered cluster */
            *cluster = 
                fatentry_buf[0] |
                ((fatentry_buf[1] & 0x0F) << 8);
        }

        if (*cluster > FAT12_MAX_CLUSTER) {
            return 1;
        }
    }

    return 0;
}

static int get_next_cluster16(struct filesystem *fs, fatcluster_t *cluster, unsigned int count)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    if (*cluster > FAT16_MAX_CLUSTER) {
        return 1;
    }

    while (count-- > 0) {
        uint32_t fatentry_idx = *cluster;
        uint32_t sector_idx = fatentry_idx / (data->sector_size >> 1);
        fatentry_idx %= data->sector_size >> 1;

        read_fat(fs, sector_idx);

        *cluster = ((uint16_t *)data->fatbuf)[fatentry_idx];
        if (*cluster > FAT16_MAX_CLUSTER) {
            return 1;
        }
    }

    return 0;
}

static int get_next_cluster32(struct filesystem *fs, fatcluster_t *cluster, unsigned int count)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    if (*cluster > FAT32_MAX_CLUSTER) {
        return 1;
    }

    while (count-- > 0) {
        uint32_t fatentry_idx = *cluster;
        uint32_t sector_idx = fatentry_idx / (data->sector_size >> 2);
        fatentry_idx %= data->sector_size >> 2;

        read_fat(fs, sector_idx);

        *cluster = ((uint32_t *)data->fatbuf)[fatentry_idx] & 0x0FFFFFFF;
        if (*cluster > FAT32_MAX_CLUSTER) {
            return 1;
        }
    }

    return 0;
}

static int get_next_cluster(struct filesystem *fs, fatcluster_t *cluster, unsigned int count)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    return data->get_next_cluster(fs, cluster, count);
}

static int match_name(
    struct fs_directory* dir,
    const char* name,
    struct fs_directory_entry* direntry)
{
    struct filesystem *fs = dir->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_dir_data *dir_data = (struct fat_dir_data *)dir->data;

    int err = fs->driver->rewind_directory(dir);
    if (err) {
        return err;
    }

    while (!fs->driver->iter_directory(dir, direntry)) {
        if (strncasecmp(name, direntry->name, sizeof(direntry->name)) == 0) {
            return 0;
        }
    }
    
    return 1;
}

static int probe(struct filesystem *fs);
static int mount(struct filesystem *fs);
static void unmount(struct filesystem *fs);

static struct fs_file *open(struct fs_directory *dir, const char *name);
static long read(struct fs_file *file, void *buf, long len);
static int seek(struct fs_file *file, long offset, int origin);
static long tell(struct fs_file *file);
static void close(struct fs_file *file);

static struct fs_directory *open_root_directory(struct filesystem *fs);
static struct fs_directory *open_directory(struct fs_directory *dir, const char *name);
static int rewind_directory(struct fs_directory *dir);
static int iter_directory(struct fs_directory *dir, struct fs_directory_entry *entry);
static void close_directory(struct fs_directory *dir);

static struct fs_driver drv = {
    .name = "fat",
    .probe = probe,
    .mount = mount,
    .unmount = unmount,
    .open = open,
    .read = read,
    .seek = seek,
    .tell = tell,
    .close = close,
    .open_root_directory = open_root_directory,
    .open_directory = open_directory,
    .rewind_directory = rewind_directory,
    .iter_directory = iter_directory,
    .close_directory = close_directory,
};

static int probe(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;

    uint8_t buf[512];

    /* read sector 0 (BPB) */
    blkif->read(blkdev, 0, buf, 1);
    const struct fat_bpb_sector *bpb = (const struct fat_bpb_sector *)buf;

    /* check signatures */
    if (bpb->signature != 0xAA55) {
        return 1;
    }
    if ((strncmp(bpb->fat.fs_type, "FAT12   ", sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat.fs_type, "FAT16   ", sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat32.fs_type, "FAT32   ", sizeof(bpb->fat32.fs_type)) != 0)) {
        return 1;
    }

    return 0;
}

static int mount(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;

    struct fat_data *data = mm_allocate(sizeof(*data));
    data->blkdev = blkdev;
    data->blkif = blkif;
    data->databuf_lba_start = -1;

    fs->data = data;

    /* read sector 0 (BPB) */
    uint8_t tmpbuf[512];
    blkif->read(blkdev, 0, tmpbuf, 1);
    const struct fat_bpb_sector *bpb = (const struct fat_bpb_sector *)tmpbuf;

    /* check signatures */
    if (bpb->signature != 0xAA55) {
        return 1;
    }
    if ((strncmp(bpb->fat.fs_type, "FAT12   ", sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat.fs_type, "FAT16   ", sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat32.fs_type, "FAT32   ", sizeof(bpb->fat32.fs_type)) != 0)) {
        return 1;
    }

    /* get information */
    data->sector_size = bpb->bytes_per_sector;
    data->sectors_per_cluster = bpb->sectors_per_cluster;
    data->cluster_size = data->sector_size * data->sectors_per_cluster;
    data->root_entry_count = bpb->root_entry_count;
    data->reserved_sectors = bpb->reserved_sector_count;
    data->root_sector_count = ((data->root_entry_count * 32) + (data->sector_size - 1)) / data->sector_size;
    data->fat_size = bpb->fat_size16 ? bpb->fat_size16 : bpb->fat32.fat_size32;
    data->total_sector_count = bpb->total_sector_count16 ? bpb->total_sector_count16 : bpb->total_sector_count32;
    data->data_area_begin = data->reserved_sectors + (bpb->fat_count * data->fat_size);
    uint32_t data_sectors = data->total_sector_count - data->data_area_begin - data->root_sector_count;
    uint32_t cluster_count = data_sectors / data->sectors_per_cluster;
    data->databuf = mm_allocate(data->cluster_size);

    /* dertermine FAT type */
    if (cluster_count < 4085) {
        data->fat_type = FT_FAT12;
        data->cluster_to_sector = cluster_to_sector12_16;
        data->sector_to_cluster = sector_to_cluster12_16;
        data->get_next_cluster = get_next_cluster12;
    } else if (cluster_count < 65525) {
        data->fat_type = FT_FAT16;
        data->cluster_to_sector = cluster_to_sector12_16;
        data->sector_to_cluster = sector_to_cluster12_16;
        data->get_next_cluster = get_next_cluster16;
    } else {
        data->fat_type = FT_FAT32;
        data->cluster_to_sector = cluster_to_sector32;
        data->sector_to_cluster = sector_to_cluster32;
        data->get_next_cluster = get_next_cluster32;
    }

    /* read FSINFO if FAT32 */
    if (data->fat_type == FT_FAT32) {
        data->root_cluster = bpb->fat32.root_cluster;

        blkif->read(blkdev, 1, tmpbuf, 1);
        const struct fat_fsinfo *fsinfo = (const struct fat_fsinfo *)tmpbuf;

        data->free_clusters = fsinfo->free_clusters;
        data->next_free_cluster = fsinfo->next_free_cluster;
    }

    return 0;
}

static void unmount(struct filesystem *fs)
{
    
}

static struct fs_file *open(struct fs_directory *dir, const char *name)
{
    struct filesystem *fs = dir->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_dir_data *dir_data = (struct fat_dir_data *)dir->data;

    struct fs_directory_entry dirent;
    if (match_name(dir, name, &dirent)) return NULL;

    if (dir_data->direntry.attribute & FAT_ATTR_DIRECTORY) return NULL;

    struct fat_file_data *file_data = mm_allocate(sizeof(*file_data));
    file_data->direntry_cluster = dir_data->current_cluster;
    file_data->direntry_entry_index = dir_data->current_entry_index;
    memcpy(&file_data->direntry, &dir_data->direntry, sizeof(dir_data->direntry));
    file_data->head_cluster = (dir_data->direntry.cluster_location_high << 16) | dir_data->direntry.cluster_location;
    file_data->current_cluster = file_data->head_cluster;
    file_data->cursor = 0;

    struct fs_file *file = mm_allocate(sizeof(*file));
    file->fs = fs;
    file->data = file_data;

    return file;
}

static long read(struct fs_file *file, void *buf, long len)
{
    struct filesystem *fs = file->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_file_data *file_data = (struct fat_file_data *)file->data;

    if (file_data->cursor + len >= file_data->direntry.size) {
        len = file_data->direntry.size - file_data->cursor;
    }
    long result = 0;

    while (len > 0) {
        long cluster_offset = file_data->cursor % data->cluster_size;
        long read_len = data->cluster_size - cluster_offset;
        if (read_len > len) {
            read_len = len;
        }

        read_cluster(fs, file_data->current_cluster);
        memcpy(buf, data->databuf + cluster_offset, read_len);
        len -= read_len;
        buf = (uint8_t *)buf + read_len;
        result += read_len;
        file_data->cursor += read_len;

        if ((cluster_offset + read_len >= data->cluster_size) &&
            get_next_cluster(fs, &file_data->current_cluster, 1)) {
            break;
        }
    }

    return result;
}

static int seek(struct fs_file *file, long offset, int origin)
{
    struct filesystem *fs = file->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_file_data *file_data = (struct fat_file_data *)file->data;

    int64_t new_cursor;
    switch (origin) {
        case SEEK_SET:
            new_cursor = offset;
            break;
        case SEEK_CUR:
            new_cursor = file_data->cursor + offset;
            break;
        case SEEK_END:
            new_cursor = file_data->direntry.size + offset;
            break;
        default:
            return 1;
    }
    if (new_cursor < 0) return 1;
    if (new_cursor > file_data->direntry.size) {
        new_cursor = file_data->direntry.size;
    }

    fatcluster_t cluster = file_data->head_cluster;
    if (get_next_cluster(fs, &cluster, new_cursor / data->cluster_size)) {
        return 1;
    }
    file_data->current_cluster = cluster;
    file_data->cursor = new_cursor;

    return 0;
}

static long tell(struct fs_file *file)
{
    struct fat_file_data *file_data = (struct fat_file_data *)file->data;

    return file_data->cursor;
}

static void close(struct fs_file *file)
{
    mm_free(file);
}

static struct fs_directory *open_root_directory(struct filesystem *fs)
{
    struct fat_data *data = (struct fat_data *)fs->data;

    struct fat_dir_data *dir_data = mm_allocate(sizeof(*dir_data));
    dir_data->head_cluster = data->fat_type == FT_FAT32 ? data->root_cluster : 0;
    dir_data->current_cluster = dir_data->head_cluster;
    dir_data->current_entry_index = 0;

    struct fs_directory *dir = mm_allocate(sizeof(*dir));
    dir->fs = fs;
    dir->data = dir_data;

    return dir;
}

static struct fs_directory *open_directory(struct fs_directory *dir, const char *name)
{
    struct filesystem *fs = dir->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_dir_data *dir_data = (struct fat_dir_data *)dir->data;

    struct fs_directory_entry dirent;
    if (match_name(dir, name, &dirent)) return NULL;

    if (!(dir_data->direntry.attribute & FAT_ATTR_DIRECTORY)) return NULL;

    struct fat_dir_data *new_dir_data = mm_allocate(sizeof(*new_dir_data));
    if (dir_data->direntry.cluster_location == 0 && dir_data->direntry.cluster_location_high == 0) {
        new_dir_data->head_cluster = data->root_cluster;
    } else {
        new_dir_data->head_cluster = (dir_data->direntry.cluster_location_high << 16) | dir_data->direntry.cluster_location;
    }
    new_dir_data->current_cluster = new_dir_data->head_cluster;
    new_dir_data->current_entry_index = 0;

    struct fs_directory *new_dir = mm_allocate(sizeof(*dir));
    new_dir->fs = fs;
    new_dir->data = new_dir_data;

    return new_dir;
}

static int rewind_directory(struct fs_directory *dir)
{
    struct fat_dir_data *dir_data = (struct fat_dir_data *)dir->data;

    dir_data->current_cluster = dir_data->head_cluster;
    dir_data->current_entry_index = 0;
    memset(&dir_data->direntry, 0, sizeof(dir_data->direntry));

    return 0;
}

static int iter_directory(struct fs_directory *dir, struct fs_directory_entry *entry)
{
    struct filesystem *fs = dir->fs;
    struct fat_data *data = (struct fat_data *)fs->data;
    struct fat_dir_data *dir_data = (struct fat_dir_data *)dir->data;

    int is_rootdir = data->fat_type != FT_FAT32 && dir_data->head_cluster == 0;
    const uint16_t block_size = is_rootdir ? data->sector_size : data->cluster_size;
    uint16_t entries_per_block = block_size / sizeof(union fat_dir_entry);
    union fat_dir_entry* entries;
    int entry_found = 0;
    uint16_t lfn_ucs2_buf[FAT_LFN_BUFLEN];
    int is_lfn = 0;

    while (!entry_found) {
        /* fetch current block (sector or cluster) */
        if (is_rootdir) {
            /* root directory */
            if (dir_data->current_entry_index / entries_per_block >= data->root_sector_count) {
                return 1;
            }
            read_sector(fs, data->data_area_begin + dir_data->current_entry_index / entries_per_block);
        } else {
            fatcluster_t current_cluster = dir_data->current_cluster;
            if (dir_data->current_entry_index >= entries_per_block) {
                if (get_next_cluster(fs, &current_cluster, 1)) {
                    return 1;
                }
                dir_data->current_entry_index = 0;
            }
            read_cluster(fs, current_cluster);
            dir_data->current_cluster = current_cluster;
        }
        entries = (union fat_dir_entry*)data->databuf;

        while (is_rootdir || dir_data->current_entry_index < entries_per_block) {
            union fat_dir_entry* current_entry =
                &entries[dir_data->current_entry_index % entries_per_block];
            if ((uint8_t)current_entry->file.name[0] == 0) {  /* End of entry list */
                return 1;
            } else if ((uint8_t)current_entry->file.name[0] == 0xE5) {
                /* skip if file entry is deleted */
            } else if ((current_entry->file.attribute & FAT_ATTR_LFNENTRY) == FAT_ATTR_LFNENTRY) {
                /* write to LFN buffer */
                get_lfn_filename(&current_entry->lfn, lfn_ucs2_buf);
                is_lfn = 1;
            } else if (current_entry->file.attribute & FAT_ATTR_VOLUME_ID) {
                /* skip if file entry is volume id */
                is_lfn = 0;
            } else {
                entry_found = 1;
                break;
            }
            dir_data->current_entry_index++;
            if (!(dir_data->current_entry_index % entries_per_block)) {
                break;
            }
        }
    }

    if (is_lfn) {
        lfn_ucs2_to_utf8(entry->name, lfn_ucs2_buf, 1, '?');
    } else {
        get_sfn_filename(
            &entries[dir_data->current_entry_index].file, entry->name);
    }
    entry->size = entries[dir_data->current_entry_index].file.size;
    memcpy(&dir_data->direntry, &entries[dir_data->current_entry_index], sizeof(dir_data->direntry));
    dir_data->current_entry_index++;
    return 0;
}

static void close_directory(struct fs_directory *dir)
{
    mm_free(dir);
}

__constructor
static void _register_driver(void)
{
    register_fs_driver(&drv);
}
