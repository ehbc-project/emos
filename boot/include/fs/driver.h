#ifndef __FS_DRIVER_H__
#define __FS_DRIVER_H__

#include <fs/fs.h>

struct fs_driver {
    struct fs_driver *next;

    const char *name;
    int (*probe)(struct filesystem *);
    int (*mount)(struct filesystem *);
    void (*unmount)(struct filesystem *);

    struct fs_file *(*open)(struct fs_directory *, const char *);
    long (*read)(struct fs_file *, void *, long);
    int (*seek)(struct fs_file *, long, int);
    long (*tell)(struct fs_file *);
    void (*close)(struct fs_file *);

    struct fs_directory *(*open_root_directory)(struct filesystem *);
    struct fs_directory *(*open_directory)(struct fs_directory *, const char *);
    int (*rewind_directory)(struct fs_directory *);
    int (*iter_directory)(struct fs_directory *, struct fs_directory_entry *);
    void (*close_directory)(struct fs_directory *);
};

void register_fs_driver(struct fs_driver *drv);

struct fs_driver *find_fs_driver(const char *name);

int fs_auto_mount(struct device *dev, const char *name);
int fs_mount(struct device *dev, const char *fsname, const char *name);

#endif // __FS_DRIVER_H__
