#ifndef __EBOOT_ASM_TEST_INSTRUCTION_H__
#define __EBOOT_ASM_TEST_INSTRUCTION_H__

#include <eboot/status.h>

status_t _i686_instruction_test(void (*test_func)(void), size_t instr_size, int *is_undefined);

#endif // __EBOOT_ASM_TEST_INSTRUCTION_H__
