#include <uacpi/kernel_api.h>

uacpi_handle uacpi_kernel_create_event() {}
void uacpi_kernel_free_event(uacpi_handle handle) {}
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {}
void uacpi_kernel_signal_event(uacpi_handle handle) {}
void uacpi_kernel_reset_event(uacpi_handle handle) {}
