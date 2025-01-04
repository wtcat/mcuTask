/*
 * Copyright (c) 2024 wtcat
 */

#ifndef SUBSYS_SD_BLKDEV_H_
#define SUBSYS_SD_BLKDEV_H_

#include "subsys/sd/sd.h"
#include "basework/linker.h"

#ifdef __cplusplus
extern "C" {
#endif

struct sd_blkinit {
    int (*handler)(struct sd_card *);
};

#define SD_BLKDEV_INIT(_handler) \
    __SDMMC_BUS_DEVICE(_handler)

#define __SDMMC_BUS_DEVICE(_handler) \
    static LINKER_ROSET_ITEM_ORDERED(sdmmc, struct sd_blkinit, \
        _handler, _order) = { \
        .handler = _handler \
   }

#ifdef SD_BLKDEV_INIT_SOURCE
LINKER_ROSET(sdmmc, struct sd_blkinit);
static void sd_blkdev_init(struct sd_card *card) {
    LINKER_SET_FOREACH(sdmmc, item, struct sd_blkinit) {
        int err = item->handler(card);
        if (err)
            pr_err("Failed to register sd blockdev (%d)\n", err);
    }
}
#endif /* SD_BLKDEV_INIT_SOURCE */

#ifdef __cplusplus
}
#endif
#endif /* SUBSYS_SD_BLKDEV_H_ */
