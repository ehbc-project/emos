#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <stdio.h>

#include <device/device.h>

struct filesystem {
    struct filesystem *next;
    
    struct fs_driver *driver;
    struct device *dev;
    char *name;

    void *data;
};

struct fs_file {
    struct filesystem *fs;

    void *data;
};

struct fs_directory {
    struct filesystem *fs;

    void *data;
};

struct fs_directory_entry {
    char name[FILENAME_MAX];
    uint64_t size;
};

struct filesystem *get_first_filesystem(void);

int register_filesystem(struct filesystem *fs, const char *name);

struct filesystem *find_filesystem(const char *name);

#endif // __FS_FS_H__
