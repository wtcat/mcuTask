/*
 * Copyright 2024 wtcat
 */

#define pr_fmt(fmt) "[ramblk]: "fmt
#include <string.h>

#include <tx_api.h>
#include <service/init.h>
#include <drivers/blkdev.h>
#include <base/log.h>

#define CONFIG_RAMBLK_SIZE 512
#define CONFIG_RAMBLK_MEMORY_SIZE 0x100000

static char ram_blk_memory[CONFIG_RAMBLK_MEMORY_SIZE];

static int 
ram_blkdev_request(struct device *dev, struct blkdev_req *req) {
    switch (req->op) {
    case BLKDEV_REQ_READ:
        memcpy(req->buffer, &ram_blk_memory[req->blkno * CONFIG_RAMBLK_SIZE], 
            req->blkcnt * CONFIG_RAMBLK_SIZE);
        return 0;
    case BLKDEV_REQ_WRITE:
        memcpy(&ram_blk_memory[req->blkno * CONFIG_RAMBLK_SIZE], req->buffer, 
            req->blkcnt * CONFIG_RAMBLK_SIZE);
        return 0;
    default:
        break;
    }
    return -EINVAL;
}

static int
ram_blkdev_control(struct device *dev, unsigned int cmd, void *arg) {
    switch (cmd) {
    case BLKDEV_IOC_GET_BLKSIZE:
        *(UINT *)arg = CONFIG_RAMBLK_SIZE;
        return 0;

    case BLKDEV_IOC_GET_BLKCOUNT:
        *(UINT *)arg = CONFIG_RAMBLK_MEMORY_SIZE / CONFIG_RAMBLK_SIZE;
        return 0;

    default:
        return -EINVAL;
    }
}

static struct block_device ram_blkdev = {
    .name = "ramblk",
    .request = ram_blkdev_request,
    .control = ram_blkdev_control
};

static int ram_blkdev_int(void) {
    int err;

    err = device_register((struct device *)&ram_blkdev);
    if (!err) {
        pr_info("%s register success\n", ram_blkdev.name);
    }
    
    return err;
}

SYSINIT(ram_blkdev_int, SI_DRIVER_LEVEL, 10);
