/*
 * Copyright 2024 wtcat
 */

#ifndef DRIVERS_DEVICE_H_
#define DRIVERS_DEVICE_H_

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

#include "tx_user.h"

#include "basework/bitops.h"
#include "basework/container/queue.h"

#ifdef __cplusplus
extern "C"{
#endif

struct device;

/*
 * Device class definition
 */
#define DEVICE_CLASS_DEFINE(_type, ...) \
    struct _type { \
        STAILQ_ENTRY(device) link; \
        const char *name; \
        void *private_data; \
        int (*control)(struct device *, unsigned int, void *); \
        __VA_ARGS__ \
    }

/* 
 * Device structure
 */
DEVICE_CLASS_DEFINE(device);

/*
 * Block device IO control command
 */
#define BLKDEV_IOC_GET_BLKSIZE         10
#define BLKDEV_IOC_GET_ERASE_BLKSIZE   11
#define BLKDEV_IOC_GET_BLKCOUNT        12
#define BLKDEV_IOC_SYNC                13

/* 
 * Device helper interface
 */
#define dev_get_private(_dev) (_dev)->private_data
#define dev_extension(_dev, _type) (_type *)((_dev) + 1)
#define to_devclass(p) (struct device *)(p)

struct device *device_find(const char *name);
int  device_register(struct device *dev);
int  device_unregister(struct device *dev);
void device_foreach(bool (*iterator)(struct device *, void *), void *user);

static inline int 
device_control(struct device *dev, unsigned int cmd, void *arg) {
    if (dev == NULL)
        return -EINVAL;
    if (dev->control)
        return dev->control(dev, cmd, arg);
    return -ENOSYS;
}


#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_DEVICE_H_ */
