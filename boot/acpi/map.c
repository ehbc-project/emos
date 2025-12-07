#include <uacpi/kernel_api.h>

#include <eboot/macros.h>
#include <eboot/mm.h>

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
    mm_map(addr, (void *)addr, ALIGN(len, 4096) >> 12, 0);

    return (void *)addr;
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    // mm_unmap(addr, ALIGN(len, 4096) >> 12);
}
