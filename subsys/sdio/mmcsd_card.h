/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 * 2024-05-24     HPMicro       add HS400 support
 * 2024-05-26     HPMicro       add UHS-I support for SD card
 */

#ifndef SUBSYS_SDIO_MMCSD_CARD_H_
#define SUBSYS_SDIO_MMCSD_CARD_H_

#include <subsys/sdio/mmcsd_host.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_SCR_BUS_WIDTH_1 (1 << 0)
#define SD_SCR_BUS_WIDTH_4 (1 << 2)

struct mmcsd_cid {
	uint8_t mid;	   /* ManufacturerID */
	uint8_t prv;	   /* Product Revision */
	uint16_t oid;	   /* OEM/Application ID */
	uint32_t psn;	   /* Product Serial Number */
	uint8_t pnm[5];	   /* Product Name */
	uint8_t reserved1; /* reserved */
	uint16_t mdt;	   /* Manufacturing Date */
	uint8_t crc;	   /* CID CRC */
	uint8_t reserved2; /* not used, always 1 */
};

struct mmcsd_csd {
	uint8_t csd_structure; /* CSD register version */
	uint8_t taac;
	uint8_t nsac;
	uint8_t tran_speed;		 /* max data transfer rate */
	uint16_t card_cmd_class; /* card command classes */
	uint8_t rd_blk_len;		 /* max read data block length */
	uint8_t rd_blk_part;
	uint8_t wr_blk_misalign;
	uint8_t rd_blk_misalign;
	uint8_t dsr_imp;	 /* DSR implemented */
	uint8_t c_size_mult; /* CSD 1.0 , device size multiplier */
	uint32_t c_size;	 /* device size */
	uint8_t r2w_factor;
	uint8_t wr_blk_len; /* max wtire data block length */
	uint8_t wr_blk_partial;
	uint8_t csd_crc;
};

struct sd_scr {
	uint8_t sd_version;
	uint8_t sd_bus_widths;
};

struct sdio_cccr {
	uint8_t sdio_version;
	uint8_t sd_version;
	uint8_t direct_cmd : 1, /*  Card Supports Direct Commands during data transfer
											only SD mode, not used for SPI mode */
		multi_block : 1,	/*  Card Supports Multi-Block */
		read_wait : 1,		/*  Card Supports Read Wait
								 only SD mode, not used for SPI mode */
		suspend_resume : 1, /*  Card supports Suspend/Resume
								 only SD mode, not used for SPI mode */
		s4mi : 1,			/* generate interrupts during a 4-bit
							   multi-block data transfer */
		e4mi : 1,			/*  Enable the multi-block IRQ during
								4-bit transfer for the SDIO card */
		low_speed : 1,		/*  Card  is  a  Low-Speed  card */
		low_speed_4 : 1;	/*  4-bit support for Low-Speed cards */

	uint8_t bus_width : 1, /* Support SDIO bus width, 1:4bit, 0:1bit */
		cd_disable : 1,	   /*  Connect[0]/Disconnect[1] the 10K-90K ohm pull-up
							   resistor on CD/DAT[3] (pin 1) of the card */
		power_ctrl : 1,	   /* Support Master Power Control */
		high_speed : 1;	   /* Support High-Speed  */
};

/*
 * SD Status
 */
union sd_status {
	uint32_t status_words[16];
	struct {
		uint32_t reserved[12];
		uint64_t : 8;
		uint64_t uhs_au_size : 4;
		uint64_t uhs_speed_grade : 4;
		uint64_t erase_offset : 2;
		uint64_t erase_timeout : 6;
		uint64_t erase_size : 16;
		uint64_t : 4;
		uint64_t au_size : 4;
		uint64_t performance_move : 8;
		uint64_t speed_class : 8;

		uint32_t size_of_protected_area;

		uint32_t sd_card_type : 16;
		uint32_t : 6;
		uint32_t : 7;
		uint32_t secured_mode : 1;
		uint32_t data_bus_width : 2;
	};
};

/*
 * SD Speed Class
 */
#define SD_SPEED_CLASS_0 0
#define SD_SPEED_CLASS_2 1
#define SD_SPEED_CLASS_4 2
#define SD_SPEED_CLASS_6 3
#define SD_SPEED_CLASS_10 4

/*
 * UHS Speed Grade
 */
#define UHS_SPEED_GRADE_0 0
#define UHS_SPEED_GRADE_1 1
#define UHS_SPEED_GRADE_3 3

struct sdio_cis {
	uint16_t manufacturer;
	uint16_t product;
	uint16_t func0_blk_size;
	uint32_t max_tran_speed;
};

/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct sdio_function_tuple {
	struct sdio_function_tuple *next;
	uint8_t code;
	uint8_t size;
	uint8_t *data;
};

struct sdio_function;
typedef void(sdio_irq_handler_t)(struct sdio_function *);

/*
 * SDIO function devices
 */
struct sdio_function {
	struct mmcsd_card *card;		 /* the card this device belongs to */
	sdio_irq_handler_t *irq_handler; /* IRQ callback */
	uint8_t num;					 /* function number */

	uint8_t func_code;	   /*  Standard SDIO Function interface code  */
	uint16_t manufacturer; /* manufacturer id */
	uint16_t product;	   /* product id */

	uint32_t max_blk_size; /* maximum block size */
	uint32_t cur_blk_size; /* current block size */

	uint32_t enable_timeout_val; /* max enable timeout in msec */

	struct sdio_function_tuple *tuples;

	void *priv;
};

#define SDIO_MAX_FUNCTIONS 7

struct mmc_ext_csd {
	uint32_t cache_size;
	uint32_t enhanced_data_strobe;
};

struct mmcsd_card {
	struct mmcsd_host *host;
	uint32_t rca;		  /* card addr */
	uint32_t resp_cid[4]; /* card CID register */
	uint32_t resp_csd[4]; /* card CSD register */
	uint32_t resp_scr[2]; /* card SCR register */

	uint16_t tacc_clks;		/* data access time by ns */
	uint32_t tacc_ns;		/* data access time by clk cycles */
	uint32_t max_data_rate; /* max data transfer rate */
	uint32_t card_capacity; /* card capacity, unit:KB */
	uint32_t card_blksize;	/* card block size */
	uint32_t card_sec_cnt;	/* card sector count*/
	uint32_t erase_size;	/* erase size in sectors */
	uint16_t card_type;
#define CARD_TYPE_MMC 0		   /* MMC card */
#define CARD_TYPE_SD 1		   /* SD card */
#define CARD_TYPE_SDIO 2	   /* SDIO card */
#define CARD_TYPE_SDIO_COMBO 3 /* SD combo (IO+mem) card */

	uint16_t flags;
#define CARD_FLAG_HIGHSPEED (1 << 0)	 /* SDIO bus speed 50MHz */
#define CARD_FLAG_SDHC (1 << 1)			 /* SDHC card */
#define CARD_FLAG_SDXC (1 << 2)			 /* SDXC card */
#define CARD_FLAG_HIGHSPEED_DDR (1 << 3) /* HIGH SPEED DDR */
#define CARD_FLAG_HS200 (1 << 4)		 /* BUS SPEED 200MHz */
#define CARD_FLAG_HS400 (1 << 5)		 /* BUS SPEED 400MHz */
#define CARD_FLAG_SDR50 (1 << 6)		 /* BUS SPEED 100MHz */
#define CARD_FLAG_SDR104 (1 << 7)		 /* BUS SPEED 200MHz */
#define CARD_FLAG_DDR50 (1 << 8)		 /* DDR50, works on 1.8V only */
	struct sd_scr scr;
	struct mmcsd_csd csd;
	uint32_t hs_max_data_rate; /* max data transfer rate in high speed mode */

	uint8_t sdio_function_num; /* total number of SDIO functions */
	struct sdio_cccr cccr;	   /* common card info */
	struct sdio_cis cis;	   /* common tuple info */
	struct sdio_function
		*sdio_function[SDIO_MAX_FUNCTIONS + 1]; /* SDIO functions (devices) */
	void *blk_dev;

	struct mmc_ext_csd ext_csd;
};

#ifdef __cplusplus
}
#endif
#endif /* SUBSYS_SDIO_MMCSD_CARD_H_ */
