/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>

#include "asm/bootinfo.h"
#include "asm/bios/disk.h"
#include "asm/bios/video.h"
#include "asm/bios/keyboard.h"
#include "asm/bios/mem.h"

#include "asm/pci/cfgspace.h"
#include "bus/pci/scan.h"
#include "acpi/rsdp.h"
#include "acpi/rsdt.h"
#include "acpi/fadt.h"

#include "term/ansi.h"

#include "bus/usb/host/xhci.h"

#include "core/panic.h"

#include "asm/io.h"

extern void main(void);

extern int font_is_glyph_full_width(uint32_t codepoint);
extern int font_get_glyph(uint32_t codepoint, uint8_t *buf, long size);

struct pci_device pci_devices[32];

static void convert_color(struct vbe_video_mode_info *vbe_mode_info, uint32_t color, uint32_t *converted)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r >>= 8 - vbe_mode_info->red_mask;
    g >>= 8 - vbe_mode_info->green_mask;
    b >>= 8 - vbe_mode_info->blue_mask;

    *converted = (r << vbe_mode_info->red_position) |
                 (g << vbe_mode_info->green_position) |
                 (b << vbe_mode_info->blue_position);
}

static void set_pixel(struct vbe_video_mode_info *vbe_mode_info, int x, int y, uint32_t color)
{
    void* pixel = (void*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);

    switch (vbe_mode_info->bpp) {
        case 4:
            *(uint8_t*)pixel &= (x & 1) ? 0x0F : 0xF0;
            *(uint8_t*)pixel |= color;
            break;
        case 8:
            *(uint8_t*)pixel = color;
            break;
        case 16:
            *(uint16_t*)pixel = color;
            break;
        case 24:
            *(uint32_t*)pixel &= 0xFF0000;
            *(uint32_t*)pixel |= color;
            break;
        case 32:
            *(uint32_t*)pixel = color;
            break;
    }
}

static void print_glyph(struct ansi_state *state, uint32_t codepoint, uint32_t foreground, uint32_t background, uint16_t x_offset, uint16_t y_offset)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    int is_full_width = font_is_glyph_full_width(codepoint);
    uint8_t glyph[32];

    if (font_get_glyph(codepoint, glyph, 32)) return;

    uint8_t *framebuffer = (uint8_t*)(vbe_mode_info->framebuffer);

    uint32_t foreground_converted, background_converted;
    convert_color(vbe_mode_info, foreground, &foreground_converted);
    convert_color(vbe_mode_info, background, &background_converted);

    for (int y = y_offset; y < y_offset + 16; y++) {
        for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
            uint8_t current_byte = glyph[(y - y_offset) * (is_full_width ? 2 : 1) + (x - x_offset) / 8];
            int fg = current_byte & (0x80 >> ((x - x_offset) % 8));

            if (state->text_reversed) {
                set_pixel(vbe_mode_info, x, y, fg ? background_converted : foreground_converted);
            } else {
                set_pixel(vbe_mode_info, x, y, fg ? foreground_converted : background_converted);
            }
        }

        if ((state->text_overlined && y == y_offset + 1) ||
            (state->text_strike && y == y_offset + 8) ||
            (state->text_underline && y == y_offset + 15)) {
            for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
                set_pixel(vbe_mode_info, x, y, state->text_reversed ? background : foreground);
            }
        }
    }
}

static void print_char(struct ansi_state *state, char ch)
{
    ansi_putchar(state, ch);
}

static void print_hex_dword(struct ansi_state *state, uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        print_char(state, hex_chars[(value >> (28 - i * 4)) & 0xF]);
    }
}

static void print_dec(struct ansi_state *state, int value)
{
    if (value < 0) {
        ansi_putchar(state, '-');
        value = -value;
    } else if (value == 0) {
        ansi_putchar(state, '0');
        return;
    }

    int digits[10];
    int i = 0;
    while (value > 0) {
        digits[i++] = value % 10;
        value /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        ansi_putchar(state, digits[j] + '0');
    }
}

static void print_str(struct ansi_state *state, const char *str)
{
    for (int i = 0; str[i]; i++) {
        ansi_putchar(state, str[i]);
    }
}

void term_draw_cursor(struct ansi_state *state)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    for (int y = state->cursor_y * 16 + 12; y < (state->cursor_y + 1) * 16; y++) {
        for (int x = state->cursor_x * 8; x < (state->cursor_x + 1) * 8; x++) {
            uint32_t *pixel = (uint32_t*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *pixel |= 0xFFFFFF;
        }
    }
}

void term_erase(struct ansi_state *state, int x0, int y0, int x1, int y1)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    for (int y = y0 * 16; y < (y1 + 1) * 16; y++) {
        for (int x = x0 * 8; x < (x1 + 1) * 8; x++) {
            set_pixel(vbe_mode_info, x, y, state->bg_color);
        }
    }
}

void term_putchar(struct ansi_state *state, char ch)
{
    print_glyph(state, ch, state->fg_color, state->bg_color, state->cursor_x * 8, state->cursor_y * 16);
}

void term_scroll(struct ansi_state *state, int n, int x0, int y0, int x1, int y1)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    if (y1 - y0 < n) {  // clear
        term_erase(state, x0, y0, x1, y1);
        return;
    }

    int start, end, direction;
    if (n < 0) {
        start = (y1 + 1) * 16;
        end = (y0 + n) * 16;
        direction = -1;
    } else {
        start = y0 * 16;
        end = (y1 + 1 - n) * 16;
        direction = 1;
    }

    for (int y = start; y != end; y += direction) {
        for (int x = x0 * 8; x <= (x1 + 1) * 8; x++) {
            uint32_t *from = (uint32_t*)(vbe_mode_info->framebuffer + (y + n * 16) * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            uint32_t *to = (uint32_t*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *to &= ~0xFFFFFF;
            *to |= *from & 0xFFFFFF;
        }
    }

    if (n > 0) {
        term_erase(state, x0, y1 - n + 1, x1, y1);
    } else {
        term_erase(state, x0, y0 + 1, x1, y0 - n);
    }
}

static void echo_char(struct ansi_state *state)
{
    char ch;
    for (;;) {
        _pc_bios_read_keyboard(NULL, &ch);
        if (ch == '\r') {
            print_str(state, "\r\n> ");
        } else if (ch == '\b') {
            ansi_putchar(state, 0x7F);
        } else {
            ansi_putchar(state, ch);
        }
    }
}

static int strncmp(const char *str1, const char *str2, int len)
{
    for (int i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
    }
    return 0;
}

static struct acpi_fadt *find_fadt(struct acpi_rsdt *rsdt)
{
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        if (strncmp(header->signature, ACPI_FADT_SIGNATURE, 4) == 0) {
            return (struct acpi_fadt *)header;
        }
    }
    return NULL;
}

static void print_str_bios(const char *str)
{
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }
}

static void print_hex_byte_bios(uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 2; i++) {
        _pc_bios_tty_output(hex_chars[(value >> (4 - i * 4)) & 0xF]);
    }
}

static void print_hex_word_bios(uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 4; i++) {
        _pc_bios_tty_output(hex_chars[(value >> (12 - i * 4)) & 0xF]);
    }
}

static void print_hex_dword_bios(uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        _pc_bios_tty_output(hex_chars[(value >> (28 - i * 4)) & 0xF]);
    }
}

static void print_dec_bios(int value)
{
    if (value < 0) {
        _pc_bios_tty_output('-');
        value = -value;
    } else if (value == 0) {
        _pc_bios_tty_output('0');
        return;
    }

    int digits[10];
    int i = 0;
    while (value > 0) {
        digits[i++] = value % 10;
        value /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        _pc_bios_tty_output(digits[j] + '0');
    }
}

__attribute__((noreturn))
void _pc_init(void)
{
    struct vbe_controller_info vbe_info;
    _pc_bios_get_vbe_controller_info(&vbe_info);

    uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;
    print_str_bios("Searching VBE Modes:\r\n");
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);
        print_str_bios("Mode ");
        print_hex_dword_bios(mode_list[i]);
        print_str_bios(": w=");
        print_dec_bios(vbe_mode_info.width);
        print_str_bios(", h=");
        print_dec_bios(vbe_mode_info.height);
        print_str_bios(", bpp=");
        print_dec_bios(vbe_mode_info.bpp);
        print_str_bios(", addr=");
        print_hex_dword_bios(vbe_mode_info.framebuffer);
        print_str_bios(", attr=");
        print_hex_word_bios(vbe_mode_info.attributes);
        print_str_bios(", mm=");
        print_hex_byte_bios(vbe_mode_info.memory_model);
        print_str_bios("\r\n");


        if (vbe_mode_info.width == 640 && vbe_mode_info.height == 480 && vbe_mode_info.bpp == 16) {
            mode = mode_list[i];
            break;
        }
    }

    if (mode == 0xFFFF) {
        panic("No supported video mode found");
    }

    _pc_bios_set_vbe_video_mode(mode);

    struct ansi_state state;
    ansi_init_state(&vbe_mode_info, vbe_mode_info.width / 8, vbe_mode_info.height / 16, &state);
    
    /* List VBE Controller Info */
    print_str(&state, " VBE Controller Info: \r\n");
    print_str(&state, "\tSignature: ");
    print_str(&state, vbe_info.signature);
    print_str(&state, "\r\n");
    print_str(&state, "\tVersion: ");
    print_hex_dword(&state, vbe_info.vbe_version);
    print_str(&state, "\r\n");
    print_str(&state, "\tOEM String: ");
    print_str(&state, (const char *)((vbe_info.oem_string.segment << 4) + vbe_info.oem_string.offset));
    print_str(&state, "\r\n");
    print_str(&state, "\tCapabilities: ");
    print_hex_dword(&state, vbe_info.capabilities);
    print_str(&state, "\r\n");
    print_str(&state, "\tOEM Vendor Name: ");
    print_str(&state, (const char *)((vbe_info.oem_vendor_name_ptr.segment << 4) + vbe_info.oem_vendor_name_ptr.offset));
    print_str(&state, "\r\n");
    print_str(&state, "\tOEM Product Name: ");
    print_str(&state, (const char *)((vbe_info.oem_product_name_ptr.segment << 4) + vbe_info.oem_product_name_ptr.offset));
    print_str(&state, "\r\n");
    print_str(&state, "\tOEM Product Rev: ");
    print_str(&state, (const char *)((vbe_info.oem_product_rev_ptr.segment << 4) + vbe_info.oem_product_rev_ptr.offset));
    print_str(&state, "\r\n");

    /* List PCI Devices */
    int pci_count = pci_host_scan(pci_devices, 32);
    print_str(&state, "PCI Devices: \r\n");
    for (int i = 0; i < pci_count; i++) {
        print_str(&state, "\tbus ");
        print_dec(&state, pci_devices[i].bus);
        print_str(&state, " device ");
        print_dec(&state, pci_devices[i].device);
        print_str(&state, " function ");
        print_dec(&state, pci_devices[i].function);
        print_str(&state, ": vendor ");
        print_hex_dword(&state, pci_devices[i].vendor_id);
        print_str(&state, ", device ");
        print_hex_dword(&state, pci_devices[i].device_id);
        print_str(&state, " (");
        print_dec(&state, pci_devices[i].class_code.base_class);
        print_str(&state, ", ");
        print_dec(&state, pci_devices[i].class_code.sub_class);
        print_str(&state, ", ");
        print_dec(&state, pci_devices[i].class_code.interface);
        print_str(&state, ")\r\n");
    }
    
    /* List Memory Map */
    uint32_t cursor = 0;
    struct smap_entry entry;

    print_str(&state, "Memory Map: \r\n");
    do {
        _pc_bios_query_address_map(&cursor, &entry, sizeof(entry));
        print_hex_dword(&state, entry.base_addr_low);
        print_char(&state, ' ');
        print_hex_dword(&state, entry.length_low);
        print_char(&state, ' ');
        print_hex_dword(&state, entry.type);
        print_str(&state, "\r\n");
    } while (cursor);

    const struct vbe_pm_interface *pm_interface;
    _pc_bios_get_vbe_pm_interface(&pm_interface);

    /* List VBE Protected Mode Interface */
    print_str(&state, "VBE Protected Mode Interface: \r\n");
    print_str(&state, "\tSet Window: ");
    if (pm_interface->set_window) {
        print_hex_dword(&state, (uint32_t)pm_interface + pm_interface->set_window);
    } else {
        print_str(&state, "Not supported");
    }
    print_str(&state, "\r\n");
    print_str(&state, "\tSet Display Start: ");
    if (pm_interface->set_display_start) {
        print_hex_dword(&state, (uint32_t)pm_interface + pm_interface->set_display_start);
    } else {
        print_str(&state, "Not supported");
    }
    print_str(&state, "\r\n");
    print_str(&state, "\tSet Primary Palette Data: ");
    if (pm_interface->set_primary_palette_data) {
        print_hex_dword(&state, (uint32_t)pm_interface + pm_interface->set_primary_palette_data);
    } else {
        print_str(&state, "Not supported");
    }
    print_str(&state, "\r\n");
    print_str(&state, "\tPort Memory Locations: ");
    if (pm_interface->port_mem_locations) {
        print_hex_dword(&state, (uint32_t)pm_interface + pm_interface->port_mem_locations);
    } else {
        print_str(&state, "Not supported");
    }
    print_str(&state, "\r\n");

    /* List ACPI Info */
    struct acpi_rsdp *rsdp = acpi_rsdp_find();
    if (rsdp) {
        print_str(&state, "RSDP found at ");
        print_hex_dword(&state, (uint32_t)rsdp);
        print_str(&state, "\r\n");
        print_str(&state, "\tVersion: ");
        print_dec(&state, rsdp->revision);
        switch (rsdp->revision) {
            case 0:
                print_str(&state, " ACPI 1.0\r\n");
                break;
            case 2:
                print_str(&state, " ACPI 2.0 or upper\r\n");
                break;
            default:
                print_str(&state, " Unknown\r\n");
                break;
        }
        print_str(&state, "\tOEM ID: ");
        print_str(&state, rsdp->oem_id);
        print_str(&state, "\r\n");

        struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_address;
        print_str(&state, "\tRSDT: ");
        print_hex_dword(&state, (uint32_t)rsdt);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tCreator ID: ");
        print_str(&state, (const char *)&rsdt->header.creator_id);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tCreator Revision: ");
        print_hex_dword(&state, rsdt->header.creator_revision);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tLength: ");
        print_hex_dword(&state, rsdt->header.length);
        print_str(&state, "\r\n");

        print_str(&state, "\tXSDT: ");
        print_hex_dword(&state, rsdp->xsdt_address);
        print_str(&state, "\r\n");

        print_str(&state, "\tTables: \r\n");
        for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
            struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
            print_str(&state, "\t- ");
            print_str(&state, header->signature);
            print_str(&state, "\r\n");
        }

        struct acpi_fadt *fadt = find_fadt(rsdt);
        if (fadt) {
            print_str(&state, "\tFADT: ");
            print_hex_dword(&state, (uint32_t)fadt);
            print_str(&state, "\r\n");

            print_str(&state, "\t\tACPI Enable: ");
            print_dec(&state, fadt->acpi_enable);
            print_str(&state, "\r\n");
            print_str(&state, "\t\tACPI Disable: ");
            print_dec(&state, fadt->acpi_disable);
            print_str(&state, "\r\n");

            print_str(&state, "\t\tPM1a Event Block: ");
            print_hex_dword(&state, fadt->pm1a_event_block);
            print_str(&state, "\r\n");
            print_str(&state, "\t\tPM1b Event Block: ");
            print_hex_dword(&state, fadt->pm1b_event_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\tPM1a Control Block: ");
            print_hex_dword(&state, fadt->pm1a_control_block);
            print_str(&state, "\r\n");
            print_str(&state, "\t\tPM1b Control Block: ");
            print_hex_dword(&state, fadt->pm1b_control_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\tPM2 Control Block: ");
            print_hex_dword(&state, fadt->pm2_control_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\tPM Timer Block: ");
            print_hex_dword(&state, fadt->pm_timer_block);
            print_str(&state, "\r\n");
        }
    }

    /* Find xHCI Controller */
    int xhci_device_index = -1;
    for (int i = 0; i < pci_count; i++) {
        if (pci_devices[i].class_code.base_class == 0x0C &&
            pci_devices[i].class_code.sub_class == 0x03 && 
            pci_devices[i].class_code.interface == 0x30) {
            print_str(&state, "xHCI Controller found at ");
            print_hex_dword(&state, pci_devices[i].vendor_id);
            print_str(&state, ", device ");
            print_hex_dword(&state, pci_devices[i].device_id);
            print_str(&state, ", function ");
            print_dec(&state, pci_devices[i].function);
            print_str(&state, "\r\n");

            xhci_device_index = i;
        }
    }

    if (xhci_device_index >= 0) {
        /* Get MMIO Address of xHCI Controller */
        uint32_t xhci_mmio_address = _bus_pci_cfg_read(pci_devices[xhci_device_index].bus, pci_devices[xhci_device_index].device, pci_devices[xhci_device_index].function, 0x10);
        print_str(&state, "xHCI MMIO Address: ");
        print_hex_dword(&state, xhci_mmio_address);
        print_str(&state, "\r\n");
    }

    /* Find EHCI Controller */
    int ehci_device_index = -1;
    for (int i = 0; i < pci_count; i++) {
        if (pci_devices[i].class_code.base_class == 0x0C &&
            pci_devices[i].class_code.sub_class == 0x03 && 
            pci_devices[i].class_code.interface == 0x20) {
            print_str(&state, "EHCI Controller found at ");
            print_hex_dword(&state, pci_devices[i].vendor_id);
            print_str(&state, ", device ");
            print_hex_dword(&state, pci_devices[i].device_id);
            print_str(&state, ", function ");
            print_dec(&state, pci_devices[i].function);
            print_str(&state, "\r\n");

            ehci_device_index = i;
        }
    }

    if (ehci_device_index >= 0) {
        /* Get MMIO Address of EHCI Controller */
        uint32_t ehci_mmio_address = _bus_pci_cfg_read(pci_devices[ehci_device_index].bus, pci_devices[ehci_device_index].device, pci_devices[ehci_device_index].function, 0x10);
        print_str(&state, "EHCI MMIO Address: ");
        print_hex_dword(&state, ehci_mmio_address);
        print_str(&state, "\r\n");
    }

    /* Find UHCI Controller */
    int uhci_device_index = -1;
    for (int i = 0; i < pci_count; i++) {
        if (pci_devices[i].class_code.base_class == 0x0C &&
            pci_devices[i].class_code.sub_class == 0x03 && 
            pci_devices[i].class_code.interface == 0x00) {
            print_str(&state, "UHCI Controller found at ");
            print_hex_dword(&state, pci_devices[i].vendor_id);
            print_str(&state, ", device ");
            print_hex_dword(&state, pci_devices[i].device_id);
            print_str(&state, ", function ");
            print_dec(&state, pci_devices[i].function);
            print_str(&state, "\r\n");

            uhci_device_index = i;
        }
    }

    if (uhci_device_index >= 0) {
        /* Get PMIO Address of UHCI Controller */
        uint32_t uhci_pmio_address = _bus_pci_cfg_read(pci_devices[uhci_device_index].bus, pci_devices[uhci_device_index].device, pci_devices[uhci_device_index].function, 0x20);
        print_str(&state, "UHCI I/O Port Address: ");
        print_hex_dword(&state, uhci_pmio_address);
        print_str(&state, "\r\n");
    }

    echo_char(&state);

    const uint16_t div = 1193180 / 1000;
    _i686_out8(0x43, 0xb6);
    _i686_out8(0x42, (uint8_t)div);
    _i686_out8(0x42, (uint8_t)(div >> 8));

    uint8_t tmp = _i686_in8(0x61);
    if (tmp != (tmp | 3)) {
        _i686_out8(0x61, tmp | 3);
    }


    main();

    panic("Kernel returned");
}
