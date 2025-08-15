#include "fs/fat.h"

#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <bswap.h>
#include <libehbcfw/syscall.h>

#define FAT_LFN_END_MASK        0x40

#define FAT12_MAX_CLUSTER       0xFF4
#define FAT16_MAX_CLUSTER       0xFFF4
#define FAT32_MAX_CLUSTER       0x0FFFFFF6

#define FAT12_BAD_CLUSTER       0xFF7
#define FAT16_BAD_CLUSTER       0xFFF7
#define FAT32_BAD_CLUSTER       0x0FFFFFF7

#define FAT12_END_CLUSTER       0xFFF
#define FAT16_END_CLUSTER       0xFFFF
#define FAT32_END_CLUSTER       0x0FFFFFFF

#define FAT_BPB_SIGNATURE       0xAA55

#define FAT_FSINFO_SIGNATURE1   0x41615252
#define FAT_FSINFO_SIGNATURE2   0x61417272
#define FAT_FSINFO_SIGNATURE3   (FAT_BPB_SIGNATURE)

#define FAT_ATTR_READ_ONLY      0x01
#define FAT_ATTR_HIDDEN         0x02
#define FAT_ATTR_SYSTEM         0x04
#define FAT_ATTR_VOLUME_ID      0x08
#define FAT_ATTR_DIRECTORY      0x10
#define FAT_ATTR_ARCHIVE        0x20

#define FAT_ATTR_LFNENTRY       (FAT_ATTR_READ_ONLY | FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM | FAT_ATTR_VOLUME_ID)

struct fat_bpb_sector {
    uint8_t         x86_jump_code[3];
    char            oem_name[8];
    uint16_t        bytes_per_sector;
    uint8_t         sectors_per_cluster;
    uint16_t        reserved_sector_count;
    uint8_t         fat_count;
    uint16_t        root_entry_count;
    uint16_t        total_sector_count16;
    uint8_t         media_type;
    uint16_t        fat_size16;
    uint16_t        sectors_per_track;
    uint16_t        head_count;
    uint32_t        hidden_sector_count;
    uint32_t        total_sector_count32;

    union {
        struct {
            uint8_t         drive_num;
            uint8_t         __reserved1;
            uint8_t         boot_signature;
            uint32_t        volume_serial;
            char            volume_label[11];
            char            fs_type[8];

            uint8_t         boot_code[448];
        } __attribute__((packed)) fat;

        struct {
            uint32_t        fat_size32;
            uint16_t        flags;
            uint16_t        version;
            uint32_t        root_cluster;
            uint16_t        fsinfo_sector;
            uint16_t        bpb_backup_sector;
            uint8_t         __reserved1[12];
            uint8_t         physical_drive_num;
            uint8_t         __reserved2;
            uint8_t         extended_boot_signature;
            uint32_t        volume_serial;
            char            volume_label[11];
            char            fs_type[8];

            uint8_t         boot_code[420];
        } __attribute__((packed)) fat32;
    } __attribute__((packed));

    uint16_t        signature;
} __attribute__((packed));

struct fat_fsinfo {
    uint32_t        signature1;
    uint8_t         __reserved1[480];
    uint32_t        signature2;
    uint32_t        free_clusters;
    uint32_t        next_free_cluster;
    uint8_t         __reserved2[14];
    uint16_t        signature3;
} __attribute__((packed));

typedef uint32_t fatcluster_t;

static int read_fat(struct fat_filesystem*, uint32_t);
static int match_name(struct fat_dir*, const char*, struct fat_direntry_file*);
static int file_iseof(struct fat_file*);

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

static int
sector_to_cluster(struct fat_filesystem* fs, fatcluster_t* cluster, lba_t lba)
{
    if (fs->fat_type == FAT_TYPE_FAT32) {
        *cluster = (lba - fs->data_area_begin) / fs->sectors_per_cluster + 2;
        return 0;
    } else if (fs->fat_type) {
        *cluster =
            (lba - fs->data_area_begin - fs->root_sector_count) /
            fs->sectors_per_cluster + 2;
        return 0;
    }
    return 1;
}

static int
cluster_to_sector(struct fat_filesystem* fs, lba_t* lba, fatcluster_t cluster)
{
    switch (fs->fat_type) {
        case FAT_TYPE_FAT12:
        case FAT_TYPE_FAT16:
            *lba =
                ((cluster - 2) * fs->sectors_per_cluster) +
                fs->data_area_begin + fs->root_sector_count;
            return 0;
        case FAT_TYPE_FAT32:
            *lba =
                ((cluster - 2) * fs->sectors_per_cluster) +
                fs->data_area_begin;
            return 0;
        default:
            return 1;
    }
}

static int
get_next_cluster(
    struct fat_filesystem* fs,
    fatcluster_t* cluster,
    uint32_t num)
{
    switch (fs->fat_type) {
        case FAT_TYPE_FAT12:
            while (num-- > 0) {
                if (*cluster > FAT12_MAX_CLUSTER) {
                    return 1;
                }

                uint16_t byte_idx = *cluster + (*cluster >> 1);
                uint16_t sector_idx = byte_idx / fs->sector_size;
                byte_idx %= fs->sector_size;

                read_fat(fs, sector_idx);

                uint8_t fatentry_buf[2];

                fatentry_buf[0] = fs->fatbuf[byte_idx];
                if (byte_idx == fs->sector_size - 1) {
                    unsigned int entry_idx;
                    read_fat(fs, sector_idx + 1);
                    fatentry_buf[1] = fs->fatbuf[0];
                } else {
                    fatentry_buf[1] = fs->fatbuf[byte_idx + 1];
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

                if (*cluster > FAT16_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        case FAT_TYPE_FAT16:
            while (num-- > 0) {
                if (*cluster > FAT16_MAX_CLUSTER) {
                    return 1;
                }

                uint32_t fatentry_idx = *cluster;
                uint32_t sector_idx = fatentry_idx / (fs->sector_size >> 1);
                fatentry_idx %= fs->sector_size >> 1;

                read_fat(fs, sector_idx);

                *cluster = CONV_LE_16(((uint16_t*)fs->fatbuf)[fatentry_idx]);
                if (*cluster > FAT16_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        case FAT_TYPE_FAT32:
            while (num-- > 0) {
                if (*cluster > FAT32_MAX_CLUSTER) {
                    return 1;
                }
                uint32_t fatentry_idx = *cluster;
                uint32_t sector_idx = fatentry_idx / (fs->sector_size >> 2);
                fatentry_idx %= fs->sector_size >> 2;

                read_fat(fs, sector_idx);

                *cluster = CONV_LE_32(((uint32_t*)fs->fatbuf)[fatentry_idx]);
                if (*cluster > FAT32_MAX_CLUSTER) {
                    return 1;
                }
            }
            break;
        default:
            return 1;
    }
    return 0;
}

/**
 * @brief Read a sector from disk
 * 
 * @param fs filesystem object struct
 * @param lba LBA address of the sector
 * @return int 0 if success, otherwise failed
 */
static int read_sector(struct fat_filesystem* fs, lba_t lba)
{
    if (fs->databuf_lba == lba) return 0;

    fs->databuf_lba = lba;
    return ehbcfw_storage_read_sectors_lba(fs->diskid, lba, 1, fs->databuf);
}

// TODO: this function reads only the first sector of a cluster
static int read_cluster(struct fat_filesystem* fs, fatcluster_t cluster)
{
    lba_t lba;
    cluster_to_sector(fs, &lba, cluster);
    if (fs->databuf_lba == lba) return 0;

    fs->databuf_lba = lba;
    return ehbcfw_storage_read_sectors_lba(fs->diskid, lba, 1, fs->databuf);
}

static int read_fat(struct fat_filesystem* fs, uint32_t sector_idx)
{
    lba_t lba = fs->reserved_sectors + sector_idx;
    if (fs->fatbuf_lba == lba) return 0;

    fs->fatbuf_lba = lba;
    return ehbcfw_storage_read_sectors_lba(fs->diskid, lba, 1, fs->fatbuf);
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
        if (i > 0x7F) {
            continue;
        } else if (i == '.') {
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
    size_t bytes_written;
    if (allow_nonascii) {
        size_t buflen = FAT_LFN_U8_BUFLEN;
        for (
            bytes_written = 0;
            bytes_written < FAT_LFN_U8_BUFLEN;
            bytes_written++) {
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
        for (
            bytes_written = 0;
            bytes_written < FAT_LFN_BUFLEN;
            bytes_written++) {
            *utf8buf++ = *ucs2buf < 0x80 ? *ucs2buf : fallback;
            ucs2buf++;
        }
        *utf8buf = 0;
    }

    return bytes_written;
}

static size_t
get_sfn_filename(
    const struct fat_direntry_file* entry,
    char *buf,
    int lowercase)
{
    size_t char_count = 0;
    for (int i = 0; i < 8 && entry->name[i] != ' '; i++) {
        buf[char_count++] =
            lowercase ? tolower(entry->name[i]) : entry->name[i];
    }
    if (entry->extension[0] != ' ') {
        buf[char_count++] = '.';
    }
    for (int i = 0; i < 3 && entry->extension[i] != ' '; i++) {
        buf[char_count++] =
            lowercase ? tolower(entry->extension[i]) : entry->extension[i];
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

int fat_mount(struct fat_filesystem *fs, int diskid)
{
    fs->diskid = diskid;
    fs->sector_size = 512;
    fs->databuf_lba = -1;
    fs->fatbuf_lba = -1;

    /* Read sector 0 (BPB) */
    read_sector(fs, 0);

    const struct fat_bpb_sector* bpb = (void*)fs->databuf;

    /* check signature */
    if (CONV_LE_16(bpb->signature) != 0xAA55) {
        return 1;
    }

    /* get information */
    fs->sector_size = CONV_LE_16(bpb->bytes_per_sector);
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->cluster_size = fs->sector_size * fs->sectors_per_cluster;
    fs->root_entry_count = CONV_LE_16(bpb->root_entry_count);
    fs->reserved_sectors = CONV_LE_16(bpb->reserved_sector_count);
    fs->root_sector_count =
        ((fs->root_entry_count * 32) + (fs->sector_size - 1)) / fs->sector_size;
    fs->fat_size = CONV_LE_16(bpb->fat_size16) ?
        CONV_LE_16(bpb->fat_size16) : CONV_LE_32(bpb->fat32.fat_size32);
    fs->total_sector_count = CONV_LE_16(bpb->total_sector_count16) ?
        CONV_LE_16(bpb->total_sector_count16) : CONV_LE_32(bpb->total_sector_count32);
    fs->data_area_begin =
        fs->reserved_sectors + (bpb->fat_count * fs->fat_size);
    uint32_t data_sectors =
        fs->total_sector_count -
        (fs->data_area_begin + fs->root_sector_count);
    uint32_t cluster_count = data_sectors / fs->sectors_per_cluster;

    /* Determine FAT type */
    if (cluster_count < 4085) {
        fs->fat_type = FAT_TYPE_FAT12;
    } else if (cluster_count < 65525) {
        fs->fat_type = FAT_TYPE_FAT16;
    } else {
        fs->fat_type = FAT_TYPE_FAT32;
    }

    /* copy volume label and serial */
    if (fs->fat_type == FAT_TYPE_FAT32) {
        memcpy(fs->volume_label, bpb->fat32.volume_label, sizeof(bpb->fat32.volume_label));
        fs->volume_serial = CONV_LE_32(bpb->fat32.volume_serial);
    } else {
        memcpy(fs->volume_label, bpb->fat.volume_label, sizeof(bpb->fat.volume_label));
        fs->volume_serial = CONV_LE_32(bpb->fat.volume_serial);
    }

    /* Read FSINFO if FAT32 */
    if (fs->fat_type == FAT_TYPE_FAT32) {
        fs->root_cluster = CONV_LE_32(bpb->fat32.root_cluster);

        read_sector(fs, 1);
        const struct fat_fsinfo* fsinfo = (void*)fs->databuf;

        fs->free_clusters = CONV_LE_32(fsinfo->free_clusters);
        fs->next_free_cluster = CONV_LE_32(fsinfo->next_free_cluster);
    }

    fs->mounted = 1;

    return 0;
}

int fat_rootdir_open(struct fat_filesystem *fs, struct fat_dir *dir)
{
    dir->fs = fs;
    dir->head_cluster = fs->fat_type == FAT_TYPE_FAT32 ? fs->root_cluster : 0;

    return 0;
}

int fat_dir_open(struct fat_dir *parent, struct fat_dir *dir, const char* name)
{
    struct fat_direntry_file dirent;
    if (match_name(parent, name, &dirent)) {
        return 1;
    }

    const uint16_t head_cluster_lo = CONV_LE_16(dirent.cluster_location);
    const uint16_t head_cluster_hi = CONV_LE_16(dirent.cluster_location_high);
    const uint32_t head_cluster = (head_cluster_hi << 16) | head_cluster_lo;

    dir->fs = parent->fs;
    dir->head_cluster = head_cluster;
    memcpy(&dir->direntry, &dirent, sizeof(dirent));

    return 0;
}

int fat_dir_iter_start(struct fat_dir* dir, struct fat_dir_iter *iter)
{
    iter->dir = dir;
    iter->current_block_idx = 0;
    iter->current_entry_idx = 0;

    return 0;
}

int fat_dir_iter_next(struct fat_dir_iter *iter)
{
    struct fat_dir* dir = iter->dir;
    struct fat_filesystem* fs = iter->dir->fs;

    const uint16_t block_size =
        (fs->fat_type != FAT_TYPE_FAT32) && (dir->head_cluster == 0) ?
            fs->sector_size : fs->cluster_size;
    uint16_t entries_per_block = block_size / sizeof(union fat_dir_entry);
    union fat_dir_entry* entries;
    int entry_found = 0;
    uint16_t lfn_ucs2_buf[FAT_LFN_BUFLEN];
    int is_lfn = 0;

    while (!entry_found) {
        if (iter->current_block_idx >= entries_per_block) {
            iter->current_block_idx++;
            iter->current_entry_idx = 0;
        }

        /* fetch current block (sector or cluster) */
        if (fs->fat_type != FAT_TYPE_FAT32 && dir->head_cluster == 0) {
            /* root directory */
            if (iter->current_block_idx >= fs->root_sector_count) return 1;
            read_sector(fs, fs->data_area_begin + iter->current_block_idx);
        } else {
            fatcluster_t current_cluster = dir->head_cluster;
            if (get_next_cluster(fs, &current_cluster, iter->current_block_idx)) {
                return 1;
            }
            read_cluster(fs, current_cluster);
        }
        entries = (union fat_dir_entry*)fs->databuf;

        while (iter->current_entry_idx < entries_per_block) {
            union fat_dir_entry* current_entry =
                &entries[iter->current_entry_idx];
            if ((uint8_t)current_entry->file.name[0] == 0) {  /* End of entry list */
                return 1;
            } else if ((uint8_t)current_entry->file.name[0] == 0xE5) {
                /* skip if file entry is deleted */
            } else if (current_entry->file.attribute & FAT_ATTR_LFNENTRY) {
                /* if current entry is LFN entry */
                if (fs->options.lfn_enabled) {
                    /* write to buffer if LFN is enabled */
                    get_lfn_filename(&current_entry->lfn, lfn_ucs2_buf);
                    is_lfn = 1;
                }  /* otherwise skip */
            } else if (current_entry->file.attribute & FAT_ATTR_VOLUME_ID) {
                /* skip if file entry is volume id */
                is_lfn = 0;
            } else {
                entry_found = 1;
                break;
            }
            iter->current_entry_idx++;
        }
    }

    if (is_lfn) {
        lfn_ucs2_to_utf8(
            it->filename,
            lfn_ucs2_buf,
            fs->options.unicode_enabled,
            fs->options.unknown_char_fallback);
    } else {
        get_sfn_filename(
            &entries[iter->current_entry_idx].file, iter->filename, 0);
    }
    memcpy(
        &iter->direntry,
        &entries[iter->current_entry_idx],
        sizeof(struct fat_direntry_file));
    iter->current_entry_idx++;
    return 0;
}

enum fat_file_type fat_dir_iter_get_type(struct fat_dir_iter* iter)
{
    return
        (iter->direntry.attribute & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY ?
            FAT_FILE : FAT_DIRECTORY;
}

time_t fat_dir_iter_get_time_created(struct fat_dir_iter *iter)
{
    struct tm time;

    time.tm_sec  = iter->direntry.created_time.second_div2 << 1;
    time.tm_sec += iter->direntry.created_tenth / 10;
    time.tm_min  = iter->direntry.created_time.minute;
    time.tm_hour = iter->direntry.created_time.hour;
    time.tm_mday = iter->direntry.created_date.day;
    time.tm_mon  = iter->direntry.created_date.month - 1;
    time.tm_year = iter->direntry.created_date.year + 80;

    return mktime(&time);
}

time_t fat_dir_iter_get_time_modified(struct fat_dir_iter *iter)
{
    struct tm time;

    time.tm_sec  = iter->direntry.modified_time.second_div2 << 1;
    time.tm_min  = iter->direntry.modified_time.minute;
    time.tm_hour = iter->direntry.modified_time.hour;
    time.tm_mday = iter->direntry.modified_date.day;
    time.tm_mon  = iter->direntry.modified_date.month - 1;
    time.tm_year = iter->direntry.modified_date.year + 80;

    return mktime(&time);
}

time_t fat_dir_iter_get_time_accessed(struct fat_dir_iter *iter)
{
    struct tm time;

    time.tm_sec  = 0;
    time.tm_min  = 0;
    time.tm_hour = 0;
    time.tm_mday = iter->direntry.accessed_date.day;
    time.tm_mon  = iter->direntry.accessed_date.month - 1;
    time.tm_year = iter->direntry.accessed_date.year + 80;

    return mktime(&time);
}

uint32_t dir_iter_get_size(struct fat_dir_iter* iter)
{
    return iter->direntry.size;
}

static int match_name(
    struct fat_dir* parent,
    const char* name,
    struct fat_direntry_file* direntry_buf)
{
    if (!parent) return 1;
    struct fat_filesystem* fs = parent->fs;

    struct fat_dir_iter iter;
    if (fat_dir_iter_start(parent, &iter)) {
        return 1;
    }

    while (!fat_dir_iter_next(&iter)) {
        if (strncasecmp(name, iter.filename, sizeof(iter.filename)) == 0) {
            memcpy(direntry_buf, &iter.direntry, sizeof(*direntry_buf));
            return 0;
        }
    }
    
    return 1;
}

int fat_file_open(struct fat_dir *parent, struct fat_file *file, const char *name)
{
    struct fat_filesystem* fs = parent->fs;

    struct fat_direntry_file dirent;
    if (match_name(parent, name, &dirent)) {
        return 1;
    }

    const uint16_t head_cluster_lo = CONV_LE_16(dirent.cluster_location);
    const uint16_t head_cluster_hi = CONV_LE_16(dirent.cluster_location_high);
    const uint32_t head_cluster = (head_cluster_hi << 16) | head_cluster_lo;

    file->fs = parent->fs;
    file->head_cluster = head_cluster;
    file->cursor = 0;
    memcpy(&file->direntry, &dirent, sizeof(dirent));

    return 0;
}

long fat_file_read(struct fat_file *file, void *buf, unsigned long size, unsigned long count)
{
    struct fat_filesystem* fs = file->fs;
    struct fat_direntry_file* entry = &file->direntry;

    if (fat_file_iseof(file)) return -1;

    uint8_t* bbuf = buf;

    fatcluster_t cluster_idx = file->head_cluster;
    get_next_cluster(fs, &cluster_idx, file->cursor / fs->cluster_size);

    uint32_t file_size = CONV_LE_32(file->direntry.size);

    for (size_t blkcnt = 0; blkcnt < count; blkcnt++) {
        if (file->cursor + size > file_size) {
            return blkcnt;
        }
        uint32_t cluster_offs = file->cursor % fs->cluster_size;
        size_t block_read_bytes = 0;

        while (block_read_bytes < size) {
            uint32_t cluster_max_read = fs->cluster_size - cluster_offs;
            uint32_t block_max_read = size - block_read_bytes;

            read_cluster(fs, cluster_idx);

            if (cluster_max_read > block_max_read) {
                memcpy(bbuf, fs->databuf + cluster_offs, block_max_read);
                block_read_bytes += block_max_read;
                bbuf += block_max_read;
                file->cursor += block_max_read;
                break;
            } else {
                memcpy(bbuf, fs->databuf + cluster_offs, cluster_max_read);
                block_read_bytes += cluster_max_read;
                cluster_offs = 0;
                bbuf += cluster_max_read;
                file->cursor += cluster_max_read;

                get_next_cluster(fs, &cluster_idx, 1);
            }
        }
    }

    return count;
}

int fat_file_seek(struct fat_file* file, long offset, int origin)
{
    uint32_t file_size = CONV_LE_32(file->direntry.size);

    switch (origin) {
        case SEEK_SET:
            if ((offset > file_size) ||
                (offset < 0)) return 1;
            file->cursor = offset;
            break;
        case SEEK_CUR:
            if ((offset + file->cursor > file_size) ||
            offset + file->cursor < 0)  return 1;
            file->cursor += offset;
            break;
        case SEEK_END:
            if ((offset > 0) ||
                (offset + file_size < 0)) return 1;
            file->cursor = file_size + offset;
            break;
        default:
            return 1;
    }
    return 0;
}

uint32_t fat_file_tell(struct fat_file *file)
{
    uint32_t file_size = CONV_LE_32(file->direntry.size);

    if (file->cursor < 0 || file->cursor > file_size) return -1;
    return file->cursor;
}

int fat_file_iseof(struct fat_file *file)
{
    uint32_t file_size = CONV_LE_32(file->direntry.size);
    return file->cursor >= file_size;
}