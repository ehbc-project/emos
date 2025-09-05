#ifndef __CORE_PANIC_H__
#define __CORE_PANIC_H__

/**
 * @brief Halts the system due to a fatal error.
 * @param message The panic message to display.
 */
__attribute__((noreturn))
void panic(const char *message);

#endif // __CORE_PANIC_H__
