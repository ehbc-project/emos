#include <stdio.h>
#include <string.h>

#include <eboot/shell.h>

#include "my_basic.h"

extern int basic_shell(void);

static int basic_handler(struct shell_instance *inst, int argc, char **argv)
{
    return basic_shell();
}

static struct command basic_command = {
    .name = "basic",
    .handler = basic_handler,
    .help_message = "Run BASIC interpreter",
};

__constructor
static void init()
{
	mb_init();

    shell_command_register(&basic_command);
}

status_t _start(int argc, char **argv)
{
    return STATUS_SUCCESS;
}

__destructor
static void deinit(void)
{
    shell_command_unregister(&basic_command);

	mb_dispose();
}
