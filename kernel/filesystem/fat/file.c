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

status_t open(struct object *obj, struct directory *dir, struct file **fileout, const char *name)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct filesystem_data *data = fs->data;
    struct directory_data *dir_data = dir->data;
    struct file *file = NULL;
    struct file_data *file_data = NULL;

    

    return STATUS_SUCCESS;
}

status_t close(struct object *obj, struct file *file)
{
    status_t status;
    struct filesystem *fs = FILESYSTEM(obj);
    struct file_data *file_data = file->data;

    emos_memory_free(file_data);
    emos_memory_free(file);

    return STATUS_SUCCESS;
}

status_t seek(struct object *obj, struct file *file, offset_t offset, int whence, offset_t *result);
status_t read(struct object *obj, struct file *file, char *buf, size_t count, size_t *result);
status_t write(struct object *obj, struct file *file, const char *buf, size_t count, size_t *result);
status_t tell(struct object *obj, struct file *file);
status_t sync(struct object *obj, struct file *file);
status_t flush(struct object *obj, struct file *file);
status_t lock(struct object *obj, struct file *file);
status_t unlock(struct object *obj, struct file *file);
status_t allocate(struct object *obj, struct file *file, size_t size);
status_t truncate(struct object *obj, struct file *file, size_t size);
