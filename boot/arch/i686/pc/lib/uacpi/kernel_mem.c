#include <uacpi/kernel_api.h>

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {}
void uacpi_kernel_unmap(void *addr, uacpi_size len) {}

void *uacpi_kernel_alloc(size_t size) {}
void uacpi_kernel_free(void *ptr) {}
