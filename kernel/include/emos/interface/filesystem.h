#ifndef __EMOS_INTERFACE_FILESYSTEM_H__
#define __EMOS_INTERFACE_FILESYSTEM_H__

#include <emos/types.h>
#include <emos/device.h>
#include <emos/limits.h>
#include <emos/uuid.h>

#define FILESYSTEM_INTERFACE_UUID UUID(0x6F, 0xA0, 0x71, 0x10, 0xFD, 0x4D, 0x5E, 0xE4, 0xA8, 0x25, 0x2F, 0xC2, 0xA4, 0x39, 0x6D, 0xA7)

struct directory {
    void *data;
};

struct file {
    void *data;
};

struct file_info {
    int flags;
};

struct filesystem_interface {
    status_t (*open)(struct object *obj, struct directory *dir, struct file **fileout, const char *name);
    status_t (*close)(struct object *obj, struct file *file);
    status_t (*seek)(struct object *obj, struct file *file, offset_t offset, int whence, offset_t *result);
    status_t (*read)(struct object *obj, struct file *file, char *buf, size_t count, size_t *result);
    status_t (*write)(struct object *obj, struct file *file, const char *buf, size_t count, size_t *result);
    status_t (*tell)(struct object *obj, struct file *file);
    status_t (*sync)(struct object *obj, struct file *file);
    status_t (*flush)(struct object *obj, struct file *file);
    status_t (*lock)(struct object *obj, struct file *file);
    status_t (*unlock)(struct object *obj, struct file *file);
    status_t (*allocate)(struct object *obj, struct file *file, size_t size);
    status_t (*truncate)(struct object *obj, struct file *file, size_t size);

    status_t (*open_root_directory)(struct object *obj, struct directory **dirout);
    status_t (*open_directory)(struct object *obj, struct directory *dir, struct directory **dirout, const char *name);
    status_t (*close_directory)(struct object *obj, struct directory *dir);
    status_t (*read_directory)(struct object *obj, struct directory *dir, char *filename_buf, size_t filename_buflen, struct file_info *fileinfo_buf);
    status_t (*create_file)(struct object *obj, struct directory *dir, const char *name);
    status_t (*remove_file)(struct object *obj, struct directory *dir, const char *name);
    status_t (*create_directory)(struct object *obj, struct directory *dir, const char *name);
    status_t (*remove_directory)(struct object *obj, struct directory *dir, const char *name);
    status_t (*move)(struct object *obj, struct directory *srcdir, const char *srcname, struct directory *destdir, const char *destname);
    status_t (*hardlink)(struct object *obj, struct directory *srcdir, const char *srcname, struct directory *destdir, const char *destname);
    status_t (*softlink)(struct object *obj, struct directory *srcdir, const char *srcname, struct directory *destdir, const char *destname);
    status_t (*unlink)(struct object *obj, struct directory *dir, const char *name);
};

#endif // __EMOS_INTERFACE_FILEYSTEM_H__
