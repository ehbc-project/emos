#include <uacpi/kernel_api.h>

uacpi_handle uacpi_kernel_create_mutex() {}
void uacpi_kernel_free_mutex(uacpi_handle handle) {}
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout) {}
void uacpi_kernel_release_mutex(uacpi_handle handle) {}
