#include <eboot/asm/apm.h>

#include <eboot/asm/bios/apm.h>
#include <eboot/asm/bios/bioscall.h>
#include <eboot/asm/farptr.h>
#include <eboot/asm/pc_gdt.h>

status_t _pc_apm_init(void)
{
    return STATUS_SUCCESS;
}
