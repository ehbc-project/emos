#include <emos/mm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emos/asm/page.h>
#include <emos/asm/instruction.h>
#include <emos/asm/intrinsics/register.h>
#include <emos/asm/intrinsics/invlpg.h>

#include <emos/macros.h>
#include <emos/panic.h>
#include <emos/log.h>

#define MODULE_NAME "vma"

static vpn_t vma_user_base_vpn;
static vpn_t vma_user_limit_vpn;
static vpn_t vma_user_alloc_vpn;

static vpn_t vma_kernel_base_vpn;
static vpn_t vma_kernel_limit_vpn;
static vpn_t vma_kernel_alloc_vpn;

status_t mm_vma_init(vpn_t user_base_vpn, vpn_t user_limit_vpn, vpn_t kernel_base_vpn, vpn_t kernel_limit_vpn)
{
    vma_user_base_vpn = user_base_vpn;
    vma_user_limit_vpn = user_limit_vpn;
    vma_user_alloc_vpn = user_base_vpn;
    
    vma_kernel_base_vpn = kernel_base_vpn;
    vma_kernel_limit_vpn = kernel_limit_vpn;
    vma_kernel_alloc_vpn = kernel_base_vpn;
    
    return STATUS_SUCCESS;
}

status_t mm_vma_get_available_kernel_page_count(size_t *page_count)
{
    if (page_count) *page_count = vma_kernel_limit_vpn - vma_kernel_base_vpn + 1;

    return STATUS_SUCCESS;
}

status_t mm_vma_get_free_kernel_page_count(size_t *page_count)
{
    if (page_count) *page_count = vma_kernel_limit_vpn - vma_kernel_alloc_vpn + 1;

    return STATUS_SUCCESS;
}

status_t mm_vma_get_available_user_page_count(size_t *page_count)
{
    if (page_count) *page_count = vma_user_limit_vpn - vma_user_base_vpn + 1;

    return STATUS_SUCCESS;
}

status_t mm_vma_get_free_user_page_count(size_t *page_count)
{
    if (page_count) *page_count = vma_user_limit_vpn - vma_user_alloc_vpn + 1;

    return STATUS_SUCCESS;
}

status_t mm_vma_allocate_page(size_t page_count, vpn_t *vpn, uint32_t alloc_flags)
{
    vpn_t new_vpn;

    if (alloc_flags & VAF_KERNEL) {
        new_vpn = vma_kernel_alloc_vpn;

        if (vma_kernel_alloc_vpn + page_count >= vma_kernel_limit_vpn) {
            return STATUS_INSUFFICIENT_MEMORY; 
        }
    
        vma_kernel_alloc_vpn += page_count;
    } else {
        new_vpn = vma_user_alloc_vpn;

        if (vma_user_alloc_vpn + page_count >= vma_user_limit_vpn) {
            return STATUS_INSUFFICIENT_MEMORY; 
        }
    
        vma_user_alloc_vpn += page_count;
    }

    if (vpn) *vpn = new_vpn;

    return STATUS_SUCCESS;
}

void mm_vma_free_page(vpn_t vpn, size_t page_count) {}
