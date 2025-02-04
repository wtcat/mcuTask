/*
 * Copyright 2024 wtcat
 */

#include <errno.h>

#include "tx_api.h"
#include "ux_api.h"
#include "ux_device_class_storage.h"
#include "drivers/blkdev.h"

#define _UX_ERR(_err) -(__ELASTERROR + (int)(_err))
#define USBMSC_LUN_MAX 1

/* Device ID */
#define STORAGE_SD  0

static struct device *storage_devlist[USBMSC_LUN_MAX];
static UX_SLAVE_CLASS_STORAGE_PARAMETER class_storage = {
    .ux_slave_class_storage_parameter_vendor_id   = (UCHAR *)"ST",
    .ux_slave_class_storage_parameter_product_id  = (UCHAR *)"ST-UDISK",
    .ux_slave_class_storage_parameter_product_rev = (UCHAR *)"1000",
    .ux_slave_class_storage_parameter_product_serial = (UCHAR *)"103521"  
};

static UINT sd_media_status(VOID *storage, ULONG lun, ULONG media_id, 
    ULONG *media_status) {
    (void) storage;
    (void) media_id;
    return 0;
}

static UINT sd_media_read(VOID *storage, ULONG lun, UCHAR *data_pointer, 
    ULONG number_blocks, ULONG lba, ULONG *media_status) {
    struct blkdev_req req;

    (void) storage;
    (void) media_status;
    if (lun > USBMSC_LUN_MAX)
        return UX_ERROR;

    req.op = BLKDEV_REQ_READ;
    req.blkno  = lba;
    req.blkcnt = number_blocks;
    req.buffer = data_pointer;
    return blkdev_request(storage_devlist[STORAGE_SD], &req);
}

static UINT sd_media_write(VOID *storage, ULONG lun, UCHAR *data_pointer, 
    ULONG number_blocks, ULONG lba, ULONG *media_status) {
    struct blkdev_req req;

    (void) storage;
    (void) media_status;
    if (lun > USBMSC_LUN_MAX)
        return UX_ERROR;

    req.op = BLKDEV_REQ_WRITE;
    req.blkno  = lba;
    req.blkcnt = number_blocks;
    req.buffer = data_pointer;
    return blkdev_request(storage_devlist[STORAGE_SD], &req);
}

static UINT sd_media_flush(VOID *storage, ULONG lun, ULONG number_blocks, 
    ULONG lba, ULONG *media_status) {
    (void) storage;
    (void) number_blocks;
    (void) lba;
    (void) media_status;

    if (lun > 1)
        return UX_ERROR;

    return device_control(storage_devlist[STORAGE_SD], BLKDEV_IOC_SYNC, NULL);
}

static int stm32_usbmsc_init(void) {
    UX_SLAVE_CLASS_STORAGE_LUN *storage;
    UINT block_num;
    UINT block_size;
    int err;

    /* SD media */
    storage_devlist[STORAGE_SD] = device_find("sdblk0");
    rte_assert(storage_devlist[STORAGE_SD] != NULL);

    storage = &class_storage.ux_slave_class_storage_parameter_lun[0];
    device_control(storage_devlist[STORAGE_SD], BLKDEV_IOC_GET_BLKCOUNT, &block_num);
    device_control(storage_devlist[STORAGE_SD], BLKDEV_IOC_GET_BLKSIZE, &block_size);
    storage->ux_slave_class_storage_media_last_lba        =  block_num - 1;
    storage->ux_slave_class_storage_media_block_length    =  block_size;
    storage->ux_slave_class_storage_media_type            =  0;
    storage->ux_slave_class_storage_media_removable_flag  =  0x80;
    storage->ux_slave_class_storage_media_status = sd_media_status;
    storage->ux_slave_class_storage_media_flush  = sd_media_flush;
    storage->ux_slave_class_storage_media_read   = sd_media_read;
    storage->ux_slave_class_storage_media_write  = sd_media_write;
    class_storage.ux_slave_class_storage_parameter_number_lun++;

    /* Register USB storage class */
    err = _ux_device_stack_class_register(_ux_system_slave_class_storage_name,
        ux_device_class_storage_entry, 1, 0, &class_storage);

    return _UX_ERR(err);
}

SYSINIT(stm32_usbmsc_init, SI_BUSDRIVER_LEVEL, 61);
