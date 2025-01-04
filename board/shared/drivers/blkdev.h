/*
 * Copyright (c) 2024 wtcat
 */
#ifndef DRIVERS_BLKDEV_H_
#define DRIVERS_BLKDEV_H_

#include "drivers/device.h"

#ifdef __cplusplus
extern "C"{
#endif

/*
 * Block device IO control command
 */
#define BLKDEV_IOC_GET_BLKSIZE         0
#define BLKDEV_IOC_GET_ERASE_BLKSIZE   1
#define BLKDEV_IOC_GET_BLKCOUNT        2
#define BLKDEV_IOC_SYNC                3

enum blkdev_request_op {
	BLKDEV_REQ_READ,
	BLKDEV_REQ_WRITE,
	BLKDEV_REQ_SYNC
};

struct blkdev_req {
    enum blkdev_request_op op;
    unsigned long blkno;
    unsigned long blkcnt;
    void *buffer;
};

/*
 * Block device structure
 */
DEVICE_CLASS_DEFINE(block_device,
    int (*request)(struct device *dev, struct blkdev_req *req);
);

static inline int 
blkdev_request(struct device *dev, struct blkdev_req *req) {
    struct block_device *bdev = (struct block_device *)dev;
    return bdev->request(dev, req);
}

#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_BLKDEV_H_ */
