#ifndef __SHELL_SHELL_H__
#define __SHELL_SHELL_H__

#include <eboot/compiler.h>

struct shell_instance;

void shell_start(void);
long shell_readline(const char *__restrict prompt, char *__restrict buf, long len);
const char *shell_parse(const char *__restrict line, char *__restrict buf, long buflen);
int shell_execute(struct shell_instance *inst, const char *line);

#endif // __SHELL_SHELL_H__
