/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_SD_INIT_PRIV_H_
#define SUBSYS_SD_INIT_PRIV_H_

#include "tx_api.h"
#include "subsys/sd/sd.h"

int sdio_card_init(struct sd_card *card);
int sdmmc_card_init(struct sd_card *card);
int mmc_card_init(struct sd_card *card);

#endif /* SUBSYS_SD_INIT_PRIV_H_ */
