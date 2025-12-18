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

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
)
{
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle)
{
    
}
