#ifndef __EBOOT_SHELL_H__
#define __EBOOT_SHELL_H__

#include <eboot/compiler.h>
#include <eboot/status.h>

struct shell_instance {
    struct filesystem *fs;
    struct fs_directory *working_dir;
    char working_dir_path[256];
};

struct command {
    struct command *next;

    const char *name;
    int (*handler)(struct shell_instance *inst, int argc, char **argv);
    const char *help_message;
};

void shell_start(void);
long shell_readline(const char *__restrict prompt, char *__restrict buf, long len);
const char *shell_parse(const char *__restrict line, char *__restrict buf, long buflen);
int shell_execute(struct shell_instance *inst, const char *line);

status_t shell_command_register(struct command *cmd);
void shell_command_unregister(struct command *cmd);

#define REGISTER_SHELL_COMMAND(name, init_func) \
    __constructor \
    static void _register_driver_##name(void) \
    { \
        init_func(); \
    }

#endif // __EBOOT_SHELL_H__
