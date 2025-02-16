/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 * 2024-05-25     HPMicro       add HS400 support
 * 2024-05-26     HPMicro       add UHS-I support
 */

#ifndef SUBSYS_SDIO_MMCSD_HOST_H_
#define SUBSYS_SDIO_MMCSD_HOST_H_

#include "tx_api.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mmcsd_io_cfg {
	uint32_t clock; /* clock rate */
	uint16_t vdd;

	/* vdd stores the bit number of the selected voltage range from below. */

	uint8_t bus_mode; /* command output mode */

#define MMCSD_BUSMODE_OPENDRAIN 1
#define MMCSD_BUSMODE_PUSHPULL 2

	uint8_t chip_select; /* SPI chip select */

#define MMCSD_CS_IGNORE 0
#define MMCSD_CS_HIGH 1
#define MMCSD_CS_LOW 2

	uint8_t power_mode; /* power supply mode */

#define MMCSD_POWER_OFF 0
#define MMCSD_POWER_UP 1
#define MMCSD_POWER_ON 2

	uint8_t bus_width; /* data bus width */

#define MMCSD_BUS_WIDTH_1 0
#define MMCSD_BUS_WIDTH_4 2
#define MMCSD_BUS_WIDTH_8 3

	unsigned char timing; /* timing specification used */

#define MMCSD_TIMING_LEGACY 0
#define MMCSD_TIMING_MMC_HS 1
#define MMCSD_TIMING_SD_HS 2
#define MMCSD_TIMING_UHS_SDR12 3
#define MMCSD_TIMING_UHS_SDR25 4
#define MMCSD_TIMING_UHS_SDR50 5
#define MMCSD_TIMING_UHS_SDR104 6
#define MMCSD_TIMING_UHS_DDR50 7
#define MMCSD_TIMING_MMC_DDR52 8
#define MMCSD_TIMING_MMC_HS200 9
#define MMCSD_TIMING_MMC_HS400 10
#define MMCSD_TIMING_MMC_HS400_ENH_DS 11

	unsigned char drv_type; /* driver type (A, B, C, D) */

#define MMCSD_SET_DRIVER_TYPE_B 0
#define MMCSD_SET_DRIVER_TYPE_A 1
#define MMCSD_SET_DRIVER_TYPE_C 2
#define MMCSD_SET_DRIVER_TYPE_D 3

	unsigned char signal_voltage;

#define MMCSD_SIGNAL_VOLTAGE_330 0
#define MMCSD_SIGNAL_VOLTAGE_180 1
#define MMCSD_SIGNAL_VOLTAGE_120 2
};

struct mmcsd_host;
struct mmcsd_req;

struct mmcsd_host_ops {
	void (*request)(struct mmcsd_host *host, struct mmcsd_req *req);
	void (*set_iocfg)(struct mmcsd_host *host, struct mmcsd_io_cfg *io_cfg);
	int32_t (*get_card_status)(struct mmcsd_host *host);
	void (*enable_sdio_irq)(struct mmcsd_host *host, int32_t en);
	int32_t (*execute_tuning)(struct mmcsd_host *host, int32_t opcode);
	int32_t (*switch_uhs_voltage)(struct mmcsd_host *host);
};

#define MMCSD_REQUEST(host, req)           (host)->ops.request(host, req)
#define MMCSD_SET_IOCFG(host, cfg)         (host)->ops.set_iocfg(host, cfg)
#define MMCSD_GET_CARD_STATUS(host)        (host)->ops.get_card_status(host)
#define MMCSD_ENABLE_SDIO_IRQ(host, en)    (host)->ops.enable_sdio_irq(host, en)
#define MMCSD_EXECUTE_TUNING(host, opcode) (host)->ops.execute_tuning(host, opcode)
#define MMCSD_SWITCH_UHS_VOLTAGE(host)     (host)->ops.switch_uhs_voltage(host)

struct mmcsd_host {
	char name[16];
	struct mmcsd_card *card;
	struct mmcsd_host_ops ops;
	uint32_t freq_min;
	uint32_t freq_max;
	struct mmcsd_io_cfg io_cfg;
	uint32_t valid_ocr;		 /* current valid OCR */
#define VDD_165_195 (1 << 7) /* VDD voltage 1.65 - 1.95 */
#define VDD_20_21 (1 << 8)	 /* VDD voltage 2.0 ~ 2.1 */
#define VDD_21_22 (1 << 9)	 /* VDD voltage 2.1 ~ 2.2 */
#define VDD_22_23 (1 << 10)	 /* VDD voltage 2.2 ~ 2.3 */
#define VDD_23_24 (1 << 11)	 /* VDD voltage 2.3 ~ 2.4 */
#define VDD_24_25 (1 << 12)	 /* VDD voltage 2.4 ~ 2.5 */
#define VDD_25_26 (1 << 13)	 /* VDD voltage 2.5 ~ 2.6 */
#define VDD_26_27 (1 << 14)	 /* VDD voltage 2.6 ~ 2.7 */
#define VDD_27_28 (1 << 15)	 /* VDD voltage 2.7 ~ 2.8 */
#define VDD_28_29 (1 << 16)	 /* VDD voltage 2.8 ~ 2.9 */
#define VDD_29_30 (1 << 17)	 /* VDD voltage 2.9 ~ 3.0 */
#define VDD_30_31 (1 << 18)	 /* VDD voltage 3.0 ~ 3.1 */
#define VDD_31_32 (1 << 19)	 /* VDD voltage 3.1 ~ 3.2 */
#define VDD_32_33 (1 << 20)	 /* VDD voltage 3.2 ~ 3.3 */
#define VDD_33_34 (1 << 21)	 /* VDD voltage 3.3 ~ 3.4 */
#define VDD_34_35 (1 << 22)	 /* VDD voltage 3.4 ~ 3.5 */
#define VDD_35_36 (1 << 23)	 /* VDD voltage 3.5 ~ 3.6 */
#define OCR_S18R (1 << 24)	 /* Switch to 1V8 Request */
	uint32_t flags;			 /* define device capabilities */
#define MMCSD_BUSWIDTH_4 (1 << 0)
#define MMCSD_BUSWIDTH_8 (1 << 1)
#define MMCSD_MUTBLKWRITE (1 << 2)
#define MMCSD_HOST_IS_SPI (1 << 3)
#define controller_is_spi(host) (host->flags & MMCSD_HOST_IS_SPI)
#define MMCSD_SUP_SDIO_IRQ (1 << 4)	 /* support signal pending SDIO IRQs */
#define MMCSD_SUP_HIGHSPEED (1 << 5) /* support high speed SDR */
#define MMCSD_SUP_DDR_3V3 (1 << 6)
#define MMCSD_SUP_DDR_1V8 (1 << 7)
#define MMCSD_SUP_DDR_1V2 (1 << 8)
#define MMCSD_SUP_HIGHSPEED_DDR                                                          \
	(MMCSD_SUP_DDR_3V3 | MMCSD_SUP_DDR_1V8 | MMCSD_SUP_DDR_1V2) /* HIGH SPEED DDR */
#define MMCSD_SUP_HS200_1V8 (1 << 9)
#define MMCSD_SUP_HS200_1V2 (1 << 10)
#define MMCSD_SUP_HS200 (MMCSD_SUP_HS200_1V2 | MMCSD_SUP_HS200_1V8) /* hs200 sdr */
#define MMCSD_SUP_NONREMOVABLE (1 << 11)
#define controller_is_removable(host) (!(host->flags & MMCSD_SUP_NONREMOVABLE))
#define MMCSD_SUP_HS400_1V8 (1 << 12)
#define MMCSD_SUP_HS400_1V2 (1 << 13)
#define MMCSD_SUP_HS400 (MMCSD_SUP_HS400_1V2 | MMCSD_SUP_HS400_1V8) /* hs400 ddr */
#define MMCSD_SUP_ENH_DS (1 << 14)
#define MMCSD_SUP_SDR50 (1 << 15)
#define MMCSD_SUP_SDR104 (1 << 16)
#define MMCSD_SUP_DDR50 (1 << 17)

	uint32_t max_seg_size;	/* maximum size of one dma segment */
	uint32_t max_dma_segs;	/* maximum number of dma segments in one request */
	uint32_t max_blk_size;	/* maximum block size */
	uint32_t max_blk_count; /* maximum block count */

	uint32_t id; /* Assigned host id */

	uint32_t spi_use_crc;
	TX_MUTEX bus_lock;
	TX_SEMAPHORE sem_ack;

	uint32_t sdio_irq_num;
	TX_SEMAPHORE sdio_irq_sem;
	TX_THREAD sdio_irq_thread;

	void *private_data;
};

#ifdef __cplusplus
}
#endif
#endif /* SUBSYS_SDIO_MMCSD_HOST_H_ */
