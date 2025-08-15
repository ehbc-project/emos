
#ifndef __FAT_H__
#define __FAT_H__

#include <stdint.h>
#include <time.h>
#include <assert.h>

#include <disk/disk.h>

#define FAT_TYPE_UNKNOWN    0
#define FAT_TYPE_FAT12      1
#define FAT_TYPE_FAT16      2
#define FAT_TYPE_FAT32      3

#define FAT_SFN_NAME            8
#define FAT_SFN_EXTENSION       3
#define FAT_SFN_LENGTH          (FAT_SFN_NAME + FAT_SFN_EXTENSION)
#define FAT_SFN_BUFLEN          FAT_SFN_LENGTH + 2  /* "filename" + '.' + "ext" + '\0' */

#ifdef BUILD_FILESYSTEM_FAT_LFN
#define FAT_LFN_LENGTH          255
#define FAT_LFN_BUFLEN          FAT_LFN_LENGTH + 1  /* "longfilename" + '\0' */
#define FAT_LFN_U8_BUFLEN       384
#define FAT_FILENAME_BUF_LEN    FAT_LFN_U8_BUFLEN

#else
#define FAT_FILENAME_BUF_LEN    FAT_SFN_BUFLEN

#endif

#define FAT_SECTOR_SIZE         512

struct fat_filesystem {
    int         diskid;

    char        volume_label[FAT_FILENAME_BUF_LEN];
    uint32_t    volume_serial;

    uint16_t    reserved_sectors;
    uint16_t    sector_size;
    uint32_t    cluster_size;
    uint8_t     sectors_per_cluster;
    uint8_t     fat_type : 2;
    uint8_t     mounted : 1;
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
    lba_t       databuf_lba;
    uint8_t     databuf[FAT_SECTOR_SIZE];
};

union fat_time {
    uint16_t raw;
    struct {
        uint16_t second_div2 : 5;
        uint16_t minute : 6;
        uint16_t hour : 5;
    } __attribute__((packed));
};

static_assert(sizeof(union fat_time) == 2, "Invalid fat_time size");

union fat_date {
    uint16_t raw;
    struct {
        uint16_t day : 5;
        uint16_t month : 4;
        uint16_t year : 7;
    } __attribute__((packed));
};

static_assert(sizeof(union fat_date) == 2, "Invalid fat_date size");

struct fat_direntry_file {
    char            name[FAT_SFN_NAME];
    char            extension[FAT_SFN_EXTENSION];
    uint8_t         attribute;
    uint8_t         __reserved;
    uint8_t         created_tenth;
    union fat_time  created_time;
    union fat_date  created_date;
    union fat_date  accessed_date;
    uint16_t        cluster_location_high;
    union fat_time  modified_time;
    union fat_date  modified_date;
    uint16_t        cluster_location;
    uint32_t        size;
} __attribute__((packed));

static_assert(sizeof(struct fat_direntry_file) == 32, "Invalid fat_direntry_file size");

struct fat_direntry_lfn {
    uint8_t         sequence_index;
    uint16_t        name_fragment1[5];
    uint8_t         attribute;
    uint8_t         __reserved;
    uint8_t         checksum;
    uint16_t        name_fragment2[6];
    uint16_t        cluster_location;
    uint16_t        name_fragment3[2];
} __attribute__((packed));

static_assert(sizeof(struct fat_direntry_lfn) == 32, "Invalid fat_direntry_file size");

union fat_dir_entry {
    struct fat_direntry_lfn lfn;
    struct fat_direntry_file file;
};

struct fat_file {
    struct fat_filesystem *fs;
    uint32_t head_cluster;
    uint32_t direntry_cluster_idx;
    uint8_t direntry_idx;
    struct fat_direntry_file direntry;
    uint32_t cursor;
};

struct fat_dir {
    struct fat_filesystem *fs;
    uint32_t head_cluster;
    struct fat_direntry_file direntry;
};

struct fat_dir_iter {
    struct fat_dir* dir;
    uint32_t current_block_idx;
    uint8_t current_entry_idx;
    char filename[FAT_FILENAME_BUF_LEN];
    struct fat_direntry_file direntry;
};

enum fat_file_type {
    FAT_FILE = 0,
    FAT_DIRECTORY
};

int fat_mount(struct fat_filesystem *fs, int diskid);

int fat_rootdir_open(struct fat_filesystem *fs, struct fat_dir *dir);
int fat_dir_open(struct fat_dir *parent, struct fat_dir *dir, const char *name);

int fat_dir_iter_start(struct fat_dir *dir, struct fat_dir_iter *iter);
int fat_dir_iter_next(struct fat_dir_iter *iter);

enum fat_file_type fat_dir_iter_get_type(struct fat_dir_iter *iter);
time_t fat_dir_iter_get_time_created(struct fat_dir_iter *iter);
time_t fat_dir_iter_get_time_modified(struct fat_dir_iter *iter);
time_t fat_dir_iter_get_time_accessed(struct fat_dir_iter *iter);
uint32_t fat_dir_iter_get_size(struct fat_dir_iter *iter);

int fat_file_open(struct fat_dir *parent, struct fat_file *file, const char *name);
long fat_file_read(struct fat_file *file, void *buf, unsigned long size, unsigned long count);
int fat_file_seek(struct fat_file *file, long offset, int origin);
uint32_t fat_file_tell(struct fat_file *file);
int fat_file_iseof(struct fat_file *file);

#endif // __FAT_H__
