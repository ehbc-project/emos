#include <stdint.h>
#include <stdalign.h>

#include <endian.h>

#include <emos/types.h>
#include <emos/status.h>
#include <emos/memory.h>
#include <emos/object.h>
#include <emos/filesystem.h>
#include <emos/interface.h>
#include <emos/interface/char.h>
#include <emos/interface/wchar.h>
#include <emos/interface/block.h>
#include <emos/interface/filesystem.h>
#include <emos/log.h>
#include <emos/ioport.h>
#include <emos/uuid.h>
#include <emos/disk.h>

#include "fat.h"


status_t read_fat_entry12(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t *value)
{
    status_t status;
    uint8_t entry_buf[2];
    unsigned int byte_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint8_t *fatbuf = NULL, *fatbuf2 = NULL;

    if (entry > FAT12_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    byte_idx = entry + (entry >> 1);
    sector_idx = byte_idx / data->sector_size;
    byte_idx %= data->sector_size;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    entry_buf[0] = fatbuf[byte_idx];

    if (byte_idx < data->sector_size - 1) {
        entry_buf[1] = fatbuf[byte_idx + 1];
    } else {
        status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, (void **)&fatbuf2);
        if (!CHECK_SUCCESS(status)) {
            emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
            goto has_error;
        }

        entry_buf[1] = fatbuf2[0];

        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, 0);
    }

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);

    if (entry & 1) {  /* odd-numbered cluster */
        *value = ((entry_buf[0] & 0xF0) >> 4) | (entry_buf[1] << 4);
    } else {  /* even-numbered cluster */
        *value = entry_buf[0] | ((entry_buf[1] & 0x0F) << 8);
    }

    return STATUS_SUCCESS;

has_error:
    if (fatbuf2) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, 0);
    }

    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
    }

    return status;
}

status_t read_fat_entry16(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t *value)
{
    status_t status;
    uint32_t entry_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint16_t *fatbuf = NULL;

    if (entry > FAT16_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    entry_idx = entry;
    sector_idx = entry_idx / (data->sector_size >> 1);
    entry_idx %= data->sector_size >> 1;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    *value = fatbuf[entry_idx];

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);

    return STATUS_SUCCESS;

has_error:
    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
    }

    return status;
}

status_t read_fat_entry32(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t *value)
{
    status_t status;
    uint32_t entry_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint32_t *fatbuf = NULL;

    if (entry > FAT32_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    entry_idx = entry;
    sector_idx = entry_idx / (data->sector_size >> 2);
    entry_idx %= data->sector_size >> 2;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    *value = fatbuf[entry_idx] & 0x0FFFFFFF;

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);

    return STATUS_SUCCESS;

has_error:
    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
}

    return status;
}

status_t write_fat_entry12(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t value)
{
    status_t status;
    unsigned int byte_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint8_t *fatbuf = NULL, *fatbuf2 = NULL;

    if (value > FAT12_END_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (entry > FAT12_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    byte_idx = entry + (entry >> 1);
    sector_idx = byte_idx / data->sector_size;
    byte_idx %= data->sector_size;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    if (byte_idx == data->sector_size - 1) {
        status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, (void **)&fatbuf2);
        if (!CHECK_SUCCESS(status)) {
            emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
            goto has_error;
        }

        if (entry & 1) {  /* odd-numbered cluster */
            fatbuf2[0] = (value & 0xFF0) >> 4;
        } else {  /* even-numbered cluster */
            fatbuf2[0] = (fatbuf2[0] & 0xF0) | ((value & 0xF00) >> 8);
        }

        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, 1);
    } else {
        if (entry & 1) {  /* odd-numbered cluster */
            fatbuf[byte_idx + 1] = (value & 0xFF0) >> 4;
        } else {  /* even-numbered cluster */
            fatbuf[byte_idx + 1] = (fatbuf[byte_idx + 1] & 0xF0) | ((value & 0xF00) >> 8);
        }
    }

    if (entry & 1) {  /* odd-numbered cluster */
        fatbuf[byte_idx] = (fatbuf[byte_idx] & 0x0F) | ((value & 0xF) << 4);
    } else {  /* even-numbered cluster */
        fatbuf[byte_idx] = value & 0xFF;
    }

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 1);
    
    return STATUS_SUCCESS;

has_error:
    if (fatbuf2) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx + 1, 0);
    }

    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
    }

    return status;
}

status_t write_fat_entry16(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t value)
{
    status_t status;
    uint32_t entry_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint16_t *fatbuf = NULL;

    if (value > FAT16_END_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (entry > FAT16_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    entry_idx = entry;
    sector_idx = entry_idx / (data->sector_size >> 1);
    entry_idx %= data->sector_size >> 1;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    fatbuf[entry_idx] = value;

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 1);

    return STATUS_SUCCESS;

has_error:
    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
    }

    return status;
}

status_t write_fat_entry32(struct filesystem *fs, int fat, fatcluster_t entry, fatcluster_t value)
{
    status_t status;
    uint32_t entry_idx, sector_idx;
    struct filesystem_data *data = (struct filesystem_data *)fs->data;
    uint32_t *fatbuf = NULL;

    if (value > FAT32_END_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (entry > FAT32_MAX_CLUSTER) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    if (fat < 0 || fat >= data->fat_count) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    entry_idx = entry;
    sector_idx = entry_idx / (data->sector_size >> 2);
    entry_idx %= data->sector_size >> 2;

    status = data->blkif->fetch(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, (void **)&fatbuf);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    fatbuf[entry_idx] &= 0xF0000000;
    fatbuf[entry_idx] |= value & 0x0FFFFFFF;

    data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 1);

    return STATUS_SUCCESS;

has_error:
    if (fatbuf) {
        data->blkif->release(OBJECT(fs->dev), data->reserved_sectors + fat * data->fat_size + sector_idx, 0);
    }

    return status;
}
