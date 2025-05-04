#include <uacpi/kernel_api.h>

uacpi_handle uacpi_kernel_create_spinlock() {}
void uacpi_kernel_free_spinlock(uacpi_handle handle) {}
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {}
void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags) {}
