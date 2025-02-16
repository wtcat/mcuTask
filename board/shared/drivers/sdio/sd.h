/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 * 2024-05-26     HPMicro       Add UHS-I support
 */

#ifndef DRIVER_SD_H_
#define DRIVER_SD_H_

#include <subsys/sdio/mmcsd_host.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 *  SWITCH_FUNC timing
 */
#define SD_SWITCH_FUNC_TIMING_DEFAULT 0
#define SD_SWITCH_FUNC_TIMING_HS      1
#define SD_SWITCH_FUNC_TIMING_SDR50   2
#define SD_SWITCH_FUNC_TIMING_SDR104  3
#define SD_SWITCH_FUNC_TIMING_DDR50   4


int mmcsd_send_if_cond(struct mmcsd_host *host, uint32_t ocr);
int mmcsd_send_app_op_cond(struct mmcsd_host *host, uint32_t ocr, uint32_t *rocr);

int mmcsd_get_card_addr(struct mmcsd_host *host, uint32_t *rca);
int mmcsd_get_scr(struct mmcsd_card *card, uint32_t *scr);

int init_sd(struct mmcsd_host *host, uint32_t ocr);

#ifdef __cplusplus
}
#endif
#endif /* DRIVER_SD_H_ */
