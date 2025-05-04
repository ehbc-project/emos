#include <uacpi/kernel_api.h>

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *request) {}
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {}
void uacpi_kernel_stall(uacpi_u8 usec) {}
uacpi_thread_id uacpi_kernel_get_thread_id() {}
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot() {}
void uacpi_kernel_sleep(uacpi_u64 msec) {}
void uacpi_kernel_log(uacpi_log_level level, const uacpi_char *msg) {}
