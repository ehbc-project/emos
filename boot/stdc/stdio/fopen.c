#include <stdio.h>
#include <string.h>

#include <path.h>
#include <mm/mm.h>
#include <fs/fs.h>
#include <fs/driver.h>

FILE *fopen(const char *__restrict path, const char *__restrict mode)
{
    struct path_iterator pathit;

    path_iter_init(&pathit, path);

    if (!pathit.element[0]) {
        return NULL;
    }

    struct filesystem *fs = find_filesystem(pathit.element);
    if (!fs) {
        return NULL;
    }

    struct fs_directory *dir = fs->driver->open_root_directory(fs);
    if (!dir) {
        return NULL;
    }

    while (!path_iter_next(&pathit)) {
        if (pathit.element[0] == '\0') {
            continue;
        }

        struct fs_directory *newdir = dir->fs->driver->open_directory(dir, pathit.element);
        if (!newdir) {
            return NULL;
        }

        dir->fs->driver->close_directory(dir);
        dir = newdir;
    }

    if (!pathit.element[0]) {
        return NULL;
    }
    
    struct fs_file *file = fs->driver->open(dir, pathit.element);
    if (!file) {
        return NULL;
    }

    dir->fs->driver->close_directory(dir);

    FILE *stream = mm_allocate(sizeof(*stream));

    stream->type = 1;
    stream->file.fs = fs;
    stream->file.file = file;

    return stream;
}

