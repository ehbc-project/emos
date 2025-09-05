#ifndef __I686_PC_BIOS_MISC_H__
#define __I686_PC_BIOS_MISC_H__

#include <stdint.h>

/**
 * @brief Check APM (Advanced Power Management) version.
 * @param version A pointer to store the APM version.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_check_apm_version(uint16_t *version);

/**
 * @brief Connect to the APM interface.
 * @param device The APM device ID.
 * @param entry_point The entry point for the APM callback.
 * @param code32_seg The 32-bit code segment.
 * @param code32_seg_len The length of the 32-bit code segment.
 * @param code16_seg The 16-bit code segment.
 * @param code16_seg_len The length of the 16-bit code segment.
 * @param data_seg The data segment.
 * @param data_seg_len The length of the data segment.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_connect_apm_interface(uint16_t device, void (*entry_point)(void), uint16_t code32_seg, uint16_t code32_seg_len, uint16_t code16_seg, uint16_t code16_seg_len, uint16_t data_seg, uint16_t data_seg_len);

/**
 * @brief Disconnect from the APM interface.
 * @param device The APM device ID.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_disconnect_apm_interface(uint16_t device);

/**
 * @brief Set the APM driver version.
 * @param device The APM device ID.
 * @param version The APM driver version.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_set_apm_driver_version(uint16_t device, uint16_t version);

/**
 * @brief Enable APM power management.
 * @param device The APM device ID.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_apm_enable_power_management(uint16_t device);

/**
 * @brief Set the APM power state.
 * @param device The APM device ID.
 * @param state The power state to set.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_apm_set_power_state(uint16_t device, uint8_t state);

void _pc_bios_bootnext(void);

#endif // __I686_PC_BIOS_MISC_H__
