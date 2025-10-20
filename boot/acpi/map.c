#include <uacpi/kernel_api.h>

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    return (void *)addr;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {}
