#ifndef __I686_PC_BIOS_MISC_H__
#define __I686_PC_BIOS_MISC_H__

#include <stdint.h>

int _pc_bios_check_apm_version(uint16_t *version);

int _pc_bios_connect_apm_interface(uint16_t device, void (*entry_point)(void), uint16_t code32_seg, uint16_t code32_seg_len, uint16_t code16_seg, uint16_t code16_seg_len, uint16_t data_seg, uint16_t data_seg_len);

int _pc_bios_disconnect_apm_interface(uint16_t device);

int _pc_bios_set_apm_driver_version(uint16_t device, uint16_t version);

int _pc_bios_apm_enable_power_management(uint16_t device);

int _pc_bios_apm_set_power_state(uint16_t device, uint8_t state);

void _pc_bios_bootnext(void);

#endif // __I686_PC_BIOS_MISC_H__
