#include <stdint.h>
#include <stdalign.h>
#include <string.h>

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


status_t probe(struct device *dev)
{
    status_t status;
    const struct block_interface *blkif = NULL;
    const struct fat_bpb_sector *bpb = NULL;
    const struct fat_fsinfo *fsinfo = NULL;
    unsigned int sector_size, sectors_per_cluster, cluster_size, fsinfo_sector;
    unsigned int root_entry_count, reserved_sectors, root_sector_count;
    unsigned int fat_size, total_sector_count, data_area_begin, cluster_count;

    status = emos_device_driver_get_interface(dev->driver, BLOCK_INTERFACE_UUID, (const void **)&blkif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_device_driver_get_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    status = blkif->fetch(OBJECT(dev), 0, (void **)&bpb);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface fetch() failed: 0x%08X\n", status);
        goto has_error;
    }

    if (le16toh(bpb->signature) != FAT_BPB_SIGNATURE) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }

    if ((strncmp(bpb->fat.fs_type, FAT12_FSTYPE_STR, sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat.fs_type, FAT16_FSTYPE_STR, sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat32.fs_type, FAT32_FSTYPE_STR, sizeof(bpb->fat32.fs_type)) != 0)) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }

    sector_size = le16toh(bpb->bytes_per_sector);
    sectors_per_cluster = bpb->sectors_per_cluster;
    cluster_size = sector_size * sectors_per_cluster;
    root_entry_count = le16toh(bpb->root_entry_count);
    reserved_sectors = le16toh(bpb->reserved_sector_count);
    root_sector_count = ((root_entry_count * 32) + (sector_size - 1)) / sector_size;
    fat_size = bpb->fat_size16 ? le16toh(bpb->fat_size16) : le32toh(bpb->fat32.fat_size32);
    total_sector_count = bpb->total_sector_count16 ? le16toh(bpb->total_sector_count16) : le32toh(bpb->total_sector_count32);
    data_area_begin = reserved_sectors + (bpb->fat_count * fat_size);
    cluster_count = (total_sector_count - data_area_begin - root_sector_count) / sectors_per_cluster;

    if (cluster_count > FAT16_MAX_CLUSTER) {  /* FAT32 */
        fsinfo_sector = bpb->fat32.fsinfo_sector;
        
        status = blkif->fetch(OBJECT(dev), fsinfo_sector, (void **)&fsinfo);
        if (!CHECK_SUCCESS(status)) {
            emos_log(LOG_ERROR, "block interface read() failed: 0x%08X\n", status);
            goto has_error;
        }

        if ((le32toh(fsinfo->signature1) != FAT_FSINFO_SIGNATURE1) ||
            (le32toh(fsinfo->signature2) != FAT_FSINFO_SIGNATURE2) ||
            (le32toh(fsinfo->signature3) != FAT_FSINFO_SIGNATURE3)) {
            status = STATUS_INVALID_SIGNATURE;
            goto has_error;
        }

        blkif->release(OBJECT(dev), fsinfo_sector, 0);
    }

    blkif->release(OBJECT(dev), 0, 0);

    return STATUS_SUCCESS;

has_error:
    if (blkif && fsinfo) {
        blkif->release(OBJECT(dev), fsinfo_sector, 0);
    }

    if (blkif && bpb) {
        blkif->release(OBJECT(dev), 0, 0);
    }

    return status;
}

status_t mount(struct filesystem **fsout, struct device *dev)
{
    status_t status;
    uint32_t cluster_count, fsinfo_sector;
    struct filesystem *fs = NULL;
    struct filesystem_data *data = NULL;
    const struct block_interface *blkif = NULL;
    const struct fat_bpb_sector *bpb = NULL;
    const struct fat_fsinfo *fsinfo = NULL;

    status = emos_filesystem_create(&fs);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_filesystem_create() failed: 0x%08X\n", status);
        goto has_error;
    }

    fs->dev = dev;

    status = emos_memory_allocate((void **)&data, sizeof(struct filesystem_data), alignof(struct filesystem_data));
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_memory_allocate() failed: 0x%08X\n", status);
        goto has_error;
    }

    dev->data = data;

    status = emos_device_driver_get_interface(dev->driver, BLOCK_INTERFACE_UUID, (const void **)&blkif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_device_driver_get_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    data->blkif = blkif;

    status = blkif->fetch(OBJECT(dev), 0, (void **)&bpb);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "block interface read() failed: 0x%08X\n", status);
        goto has_error;
    }

    if (le16toh(bpb->signature) != FAT_BPB_SIGNATURE) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }

    if ((strncmp(bpb->fat.fs_type, FAT12_FSTYPE_STR, sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat.fs_type, FAT16_FSTYPE_STR, sizeof(bpb->fat.fs_type)) != 0) &&
        (strncmp(bpb->fat32.fs_type, FAT32_FSTYPE_STR, sizeof(bpb->fat32.fs_type)) != 0)) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }

    data->sector_size = le16toh(bpb->bytes_per_sector);
    data->sectors_per_cluster = bpb->sectors_per_cluster;
    data->cluster_size = data->sector_size * data->sectors_per_cluster;
    data->root_entry_count = le16toh(bpb->root_entry_count);
    data->reserved_sectors = le16toh(bpb->reserved_sector_count);
    data->root_sector_count = ((data->root_entry_count * 32) + (data->sector_size - 1)) / data->sector_size;
    data->fat_size = bpb->fat_size16 ? le16toh(bpb->fat_size16) : le32toh(bpb->fat32.fat_size32);
    data->total_sector_count = bpb->total_sector_count16 ? le16toh(bpb->total_sector_count16) : le32toh(bpb->total_sector_count32);
    data->fat_count = bpb->fat_count;
    data->data_area_begin = data->reserved_sectors + (data->fat_count * data->fat_size);
    cluster_count = (data->total_sector_count - data->data_area_begin - data->root_sector_count) / data->sectors_per_cluster;

    if (cluster_count <= FAT12_MAX_CLUSTER) {
        data->fat_type = FT_FAT12;
        data->read_fat_entry = read_fat_entry12;
        data->write_fat_entry = write_fat_entry12;
    } else if (cluster_count <= FAT16_MAX_CLUSTER) {
        data->fat_type = FT_FAT16;
        data->read_fat_entry = read_fat_entry16;
        data->write_fat_entry = write_fat_entry16;
    } else {
        data->fat_type = FT_FAT32;
        data->read_fat_entry = read_fat_entry32;
        data->write_fat_entry = write_fat_entry32;
    }

    if (data->fat_type == FT_FAT32) {
        data->root_sector_count = 0;
        data->root_cluster = bpb->fat32.root_cluster;
        fsinfo_sector = bpb->fat32.fsinfo_sector;
        data->fsinfo_sector = fsinfo_sector;

        status = blkif->fetch(OBJECT(dev), data->fsinfo_sector, (void **)&fsinfo);
        if (!CHECK_SUCCESS(status)) {
            emos_log(LOG_ERROR, "block interface read() failed: 0x%08X\n", status);
            goto has_error;
        }

        if ((le32toh(fsinfo->signature1) != FAT_FSINFO_SIGNATURE1) ||
            (le32toh(fsinfo->signature2) != FAT_FSINFO_SIGNATURE2) ||
            (le32toh(fsinfo->signature3) != FAT_FSINFO_SIGNATURE3)) {
            status = STATUS_INVALID_SIGNATURE;
            goto has_error;
        }

        data->free_clusters = fsinfo->free_clusters;
        data->next_free_cluster = fsinfo->next_free_cluster;

        blkif->release(OBJECT(dev), data->fsinfo_sector, 0);
    }

    blkif->release(OBJECT(dev), 0, 0);

    if (fsout) *fsout = fs;

    return STATUS_SUCCESS;

has_error:
    if (blkif && fsinfo) {
        blkif->release(OBJECT(dev), fsinfo_sector, 0);
    }

    if (blkif && bpb) {
        blkif->release(OBJECT(dev), 0, 0);
    }

    if (data) {
        emos_memory_free(data);
    }

    if (fs) {
        emos_filesystem_remove(fs);
    }

    return status;
}

status_t unmount(struct filesystem *fs)
{
    struct filesystem_data *data = fs->data;

    emos_memory_free(data);
    emos_filesystem_remove(fs);

    return STATUS_SUCCESS;
}
