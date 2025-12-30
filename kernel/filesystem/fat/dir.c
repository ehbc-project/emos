#include <stdint.h>
#include <stdalign.h>

#include <endian.h>

#include <emos/types.h>
#include <emos/status.h>
#include <emos/memory.h>
#include <emos/object.h>
#include <emos/filesystem.h>
#include <emos/interface.h>
#include <emos/interface/char.h>
#include <emos/interface/wchar.h>
#include <emos/interface/block.h>
#include <emos/interface/filesystem.h>
#include <emos/log.h>
#include <emos/ioport.h>
#include <emos/uuid.h>
#include <emos/disk.h>

#include "fat.h"

status_t open_root_directory(struct object *obj, struct directory **dirout)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory *dir = NULL;
    struct directory_data *dir_data = NULL;

    status = emos_memory_allocate((void **)&dir, sizeof(struct directory), alignof(struct directory));
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_memory_allocate() failed: 0x%08X\n", status);
        goto has_error;
    }

    status = emos_memory_allocate((void **)&dir_data, sizeof(struct directory_data), alignof(struct directory_data));
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_memory_allocate() failed: 0x%08X\n", status);
        goto has_error;
    }

    dir->data = dir_data;

    dir_data->head_cluster = data->fat_type == FT_FAT32 ? data->root_cluster : 0;
    dir_data->current_cluster = dir_data->head_cluster;
    dir_data->current_entry_index = 0;

    if (dirout) *dirout = dir;

    return STATUS_SUCCESS;

has_error:
    if (dir_data) {
        emos_memory_free(dir_data);
    }

    if (dir) {
        emos_memory_free(dir);
    }

    return status;
}

status_t open_directory(struct object *obj, struct directory *dir, struct directory **dirout, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;
    struct directory *new_dir = NULL;
    struct directory_data *new_dir_data = NULL;


    return STATUS_SUCCESS;
}

status_t close_directory(struct object *obj, struct directory *dir)
{
    status_t status;
    struct directory_data *dir_data = dir->data;

    emos_memory_free(dir_data);
    emos_memory_free(dir);

    return STATUS_SUCCESS;
}

status_t read_directory(struct object *obj, struct directory *dir, char *filename_buf, size_t filename_buflen, struct file_info *fileinfo_buf)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;
    

    return STATUS_SUCCESS;
}

status_t create_file(struct object *obj, struct directory *dir, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;
    

    return STATUS_SUCCESS;
}

status_t remove_file(struct object *obj, struct directory *dir, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;

        

    return STATUS_SUCCESS;
}

status_t create_directory(struct object *obj, struct directory *dir, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;

        

    return STATUS_SUCCESS;
}

status_t remove_directory(struct object *obj, struct directory *dir, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;

        

    return STATUS_SUCCESS;
}

status_t move(struct object *obj, struct directory *srcdir, const char *srcname, struct directory *destdir, const char *destname)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *srcdir_data = srcdir->data;
    struct directory_data *destdir_data = destdir->data;

    

    return STATUS_SUCCESS;
}
