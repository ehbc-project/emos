#include <fs/fs.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mm/mm.h>

static struct filesystem *fs_list_head = NULL;

struct filesystem *get_first_filesystem(void)
{
    return fs_list_head;
}

int register_filesystem(struct filesystem *__restrict fs, const char *__restrict name)
{
    if (!fs->driver) return 1;
    
    struct filesystem *conflicting_fs = find_filesystem(name);
    if (conflicting_fs) return 1;

    if (!fs_list_head) {
        fs_list_head = fs;
    } else {
        struct filesystem *current = fs_list_head;
        while (current->next) {
            current = current->next;
        }
        current->next = fs;
    }

    fs->next = NULL;
    fs->name = mm_allocate(strlen(name) + 1);
    strcpy(fs->name, name);
    
    return 0;
}

struct filesystem *find_filesystem(const char *name)
{
    struct filesystem *current = fs_list_head;
    
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        
        current = current->next;
    }

    return NULL;
}
