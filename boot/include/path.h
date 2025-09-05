#ifndef __PATH_H__
#define __PATH_H__

#include <stdio.h>

struct path_iterator {
    const char *path;
    const char *cursor;
    char element[FILENAME_MAX];
    int has_separator;
};

void path_iter_init(struct path_iterator *it, const char *path);
int path_iter_next(struct path_iterator *it);

char *path_join(char *dest, size_t len, const char *src);
char *path_normalize(char *dest, size_t len, const char *src);

char *path_get_fsname(char *buf, size_t len, const char *path);
char *path_get_dirname(char *buf, size_t len, const char *path);
char *path_get_basename(char *buf, size_t len, const char *path);
char *path_get_stem(char *buf, size_t len, const char *path);
char *path_get_extension(char *buf, size_t len, const char *path);

int path_is_absolute(const char *path);
int path_compare(const char *path1, const char *path2, int case_sensitive);

#endif // __PATH_H__
