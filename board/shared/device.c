/*
 * Copyright (c) 2024 wtcat
 */

#include <errno.h>
#include <string.h>

#include <tx_api.h>
#include <service/init.h>
#include <drivers/device.h>

static STAILQ_HEAD(device_list, device) __device_head;	
static TX_MUTEX dev_mutex;

static struct device *device_find_locked(const char *name) {
    struct device *dev;

    STAILQ_FOREACH(dev, &__device_head, link) {
        if (!strcmp(name, dev->name))
            return dev;
    }
    return NULL;
}

struct device *device_find(const char *name) {
    if (name == NULL)
        return NULL;

    guard(os_mutex)(&dev_mutex);
    return device_find_locked(name);
}

int device_register(struct device *dev) {
    if (dev == NULL)
        return -EINVAL;

    guard(os_mutex)(&dev_mutex);
    if (dev->name == NULL)
        return -EINVAL;

    if (device_find_locked(dev->name))
        return -EEXIST;

    STAILQ_INSERT_TAIL(&__device_head, dev, link);
    return 0;
}

int device_unregister(struct device *dev) {
    if (dev == NULL)
        return -EINVAL;

    guard(os_mutex)(&dev_mutex);
    if (dev->name == NULL)
        return -EINVAL;

    if (device_find_locked(dev->name)) {
        STAILQ_REMOVE(&__device_head, dev, device, link);
        return 0;
    }

    return -ENODEV;
}

void device_foreach(bool (*iterator)(struct device *, void *), void *user) {
    struct device *dev;

    if (iterator == NULL)
        return;

    guard(os_mutex)(&dev_mutex);
    STAILQ_FOREACH(dev, &__device_head, link) {
        if (iterator(dev, user))
            break;
    }
}

static int device_init(void) {
    STAILQ_INIT(&__device_head);
    tx_mutex_create(&dev_mutex, "device", TX_INHERIT);
    return 0;
}

SYSINIT(device_init, SI_EARLY_LEVEL, 00);
