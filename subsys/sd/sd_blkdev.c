/*
 * Copyright 2024 wtcat
 */

#define pr_fmt(fmt) "[sdblk]: "fmt
#include "tx_api.h"
#include "basework/log.h"

#include "subsys/sd/sd.h"
#include "subsys/sd/private/sd_ops.h"
#include "subsys/sd/sd_blkdev.h"

#include "drivers/blkdev.h"

static int 
sd_blkdev_request(struct device *dev, struct blkdev_req *req) {
    struct sd_card *card = dev_get_private(dev);

    switch (req->op) {
    case BLKDEV_REQ_READ:
        return card_read_blocks(card, req->buffer, req->blkno, req->blkcnt);
    case BLKDEV_REQ_WRITE:
        return card_write_blocks(card, req->buffer, req->blkno, req->blkcnt);
    default:
        break;
    }
    return -EINVAL;
}

static int
sd_blkdev_control(struct device *dev, unsigned int cmd, void *arg) {
    struct sd_card *card = dev_get_private(dev);
    
    return card_ioctl(card, (uint8_t)cmd, arg);
}

static int sd_blkdev_register(struct sd_card *card) {
    static uint8_t devno;
    struct block_device *bdev;
    char name[] = {"sdblk0"};
    char *p;
    int err;
    
    bdev = kzalloc(sizeof(*bdev) + sizeof(name) + 1, 0);
    if (bdev == NULL)
        return -ENOMEM;
    
    name[5] += devno;
    p = (char *)(bdev + 1);
    strcpy(p, name);

    bdev->private_data = card;
    bdev->name = p;
    bdev->request = sd_blkdev_request;
    bdev->control = sd_blkdev_control;
    err = device_register((struct device *)bdev);
    if (!err) {
        pr_info("%s register success\n", name);
        devno++;
    }
    
    return err;
}

SD_BLKDEV_INIT(sd_blkdev_register);
