/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-06-15     hichard      first version
 * 2024-05-25     HPMicro      add HS400 support
 */

#define pr_fmt(fmt) "[SDIO]: " fmt "\n"

#include <errno.h>
#include <string.h>

#include <tx_api.h>
#include <basework/log.h>
#include <drivers/sdio/mmc.h>
#include <drivers/sdio/mmcsd_core.h>


static const uint32_t tran_unit[] = {10000, 100000, 1000000, 10000000, 0, 0, 0, 0};

static const uint8_t tran_value[] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80,
};

static const uint32_t tacc_uint[] = {
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
};

static const uint8_t tacc_value[] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80,
};

static inline uint32_t GET_BITS(uint32_t *resp, uint32_t start, uint32_t size) {
	const int32_t __size = size;
	const uint32_t __mask = (__size < 32 ? 1 << __size : 0) - 1;
	const int32_t __off = 3 - ((start) / 32);
	const int32_t __shft = (start) & 31;
	uint32_t __res;

	__res = resp[__off] >> __shft;
	if (__size + __shft > 32)
		__res |= resp[__off - 1] << ((32 - __shft) % 32);

	return __res & __mask;
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static int32_t mmcsd_parse_csd(struct mmcsd_card *card) {
	uint32_t a, b;
	struct mmcsd_csd *csd = &card->csd;
	uint32_t *resp = card->resp_csd;

	/*
	 * We only understand CSD structure v1.1 and v1.2.
	 * v1.2 has extra information in bits 15, 11 and 10.
	 * We also support eMMC v4.4 & v4.41.
	 */
	csd->csd_structure = GET_BITS(resp, 126, 2);
	if (csd->csd_structure == 0) {
		pr_err("unrecognised CSD structure version %d!", csd->csd_structure);
		return -EINVAL;
	}

	csd->taac = GET_BITS(resp, 112, 8);
	csd->nsac = GET_BITS(resp, 104, 8);
	csd->tran_speed = GET_BITS(resp, 96, 8);
	csd->card_cmd_class = GET_BITS(resp, 84, 12);
	csd->rd_blk_len = GET_BITS(resp, 80, 4);
	csd->rd_blk_part = GET_BITS(resp, 79, 1);
	csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
	csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
	csd->dsr_imp = GET_BITS(resp, 76, 1);
	csd->c_size = GET_BITS(resp, 62, 12);
	csd->c_size_mult = GET_BITS(resp, 47, 3);
	csd->r2w_factor = GET_BITS(resp, 26, 3);
	csd->wr_blk_len = GET_BITS(resp, 22, 4);
	csd->wr_blk_partial = GET_BITS(resp, 21, 1);
	csd->csd_crc = GET_BITS(resp, 1, 7);

	card->card_blksize = 1 << csd->rd_blk_len;
	card->tacc_clks = csd->nsac * 100;
	card->tacc_ns =
		(tacc_uint[csd->taac & 0x07] * tacc_value[(csd->taac & 0x78) >> 3] + 9) / 10;
	card->max_data_rate =
		tran_unit[csd->tran_speed & 0x07] * tran_value[(csd->tran_speed & 0x78) >> 3];
	if (csd->wr_blk_len >= 9) {
		a = GET_BITS(resp, 42, 5);
		b = GET_BITS(resp, 37, 5);
		card->erase_size = (a + 1) * (b + 1);
		card->erase_size <<= csd->wr_blk_len - 9;
	}

	return 0;
}

/*
 * Read extended CSD.
 */
static int mmc_get_ext_csd(struct mmcsd_card *card, uint8_t **new_ext_csd) {
	void *ext_csd;
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;

	*new_ext_csd = NULL;
	if (GET_BITS(card->resp_csd, 122, 4) < 4)
		return 0;

	/*
	 * As the ext_csd is so large and mostly unused, we don't store the
	 * raw block in mmc_card.
	 */
	ext_csd = kmalloc(512, GMF_KERNEL);
	if (!ext_csd) {
		pr_err("alloc memory failed when get ext csd!");
		return -ENOMEM;
	}

	memset(&req, 0, sizeof(struct mmcsd_req));
	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	memset(&data, 0, sizeof(struct mmcsd_data));
	req.cmd = &cmd;
	req.data = &data;
	cmd.cmd_code = SEND_EXT_CSD;
	cmd.arg = 0;

	/* NOTE HACK:  the RESP_SPI_R1 is always correct here, but we
	 * rely on callers to never use this with "native" calls for reading
	 * CSD or CID.  Native versions of those commands use the R2 type,
	 * not R1 plus a data block.
	 */
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
	data.blksize = 512;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = ext_csd;

	/*
	 * Some cards require longer data read timeout than indicated in CSD.
	 * Address this by setting the read timeout to a "reasonably high"
	 * value. For the cards tested, 300ms has proven enough. If necessary,
	 * this value can be increased if other problematic cards require this.
	 */
	data.timeout_ns = 300000000;
	data.timeout_clks = 0;
	mmcsd_send_request(card->host, &req);
	if (cmd.err)
		return cmd.err;
	if (data.err)
		return data.err;

	*new_ext_csd = ext_csd;
	return 0;
}

/*
 * Decode extended CSD.
 */
static int mmc_parse_ext_csd(struct mmcsd_card *card, uint8_t *ext_csd) {
	uint64_t card_capacity = 0;
	struct mmcsd_host *host;
	if (card == NULL || ext_csd == NULL) {
		pr_err("emmc parse ext csd fail, invaild args");
		return -1;
	}

	host = card->host;

	uint8_t device_type = ext_csd[EXT_CSD_CARD_TYPE];
	if ((host->flags & MMCSD_SUP_HS400) && (device_type & EXT_CSD_CARD_TYPE_HS400)) {
		card->flags |= CARD_FLAG_HS400;
		card->max_data_rate = 200000000;
	} else if ((host->flags & MMCSD_SUP_HS200) &&
			   (device_type & EXT_CSD_CARD_TYPE_HS200)) {
		card->flags |= CARD_FLAG_HS200;
		card->max_data_rate = 200000000;
	} else if ((host->flags & MMCSD_SUP_HIGHSPEED_DDR) &&
			   (device_type & EXT_CSD_CARD_TYPE_DDR_52)) {
		card->flags |= CARD_FLAG_HIGHSPEED_DDR;
		card->hs_max_data_rate = 52000000;
	} else {
		card->flags |= CARD_FLAG_HIGHSPEED;
		card->hs_max_data_rate = 52000000;
	}

	if (ext_csd[EXT_CSD_STROBE_SUPPORT] != 0) {
		card->ext_csd.enhanced_data_strobe = 1;
	}

	card->ext_csd.cache_size =
		ext_csd[EXT_CSD_CACHE_SIZE + 0] << 0 | ext_csd[EXT_CSD_CACHE_SIZE + 1] << 8 |
		ext_csd[EXT_CSD_CACHE_SIZE + 2] << 16 | ext_csd[EXT_CSD_CACHE_SIZE + 3] << 24;

	card_capacity = *((uint32_t *)&ext_csd[EXT_CSD_SEC_CNT]);
	card->card_sec_cnt = card_capacity;
	card_capacity *= card->card_blksize;
	card_capacity >>= 10; /* unit:KB */
	card->card_capacity = card_capacity;
	pr_info("emmc card capacity %d KB, card sec count:%d.", card->card_capacity,
			card->card_sec_cnt);

	return 0;
}

/**
 *   mmc_switch - modify EXT_CSD register
 *   @card: the MMC card associated with the data transfer
 *   @set: cmd set values
 *   @index: EXT_CSD register index
 *   @value: value to program into EXT_CSD register
 *
 *   Modifies the EXT_CSD register for selected card.
 */
static int mmc_switch(struct mmcsd_card *card, uint8_t set, uint8_t index,
					  uint8_t value) {
	int err;
	struct mmcsd_host *host = card->host;
	struct mmcsd_cmd cmd = {0};

	cmd.cmd_code = SWITCH;
	cmd.arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) | (index << 16) | (value << 8) | set;
	cmd.flags = RESP_R1B | CMD_AC;
	err = mmcsd_send_cmd(host, &cmd, 3);
	if (err)
		return err;

	return 0;
}

static int mmc_compare_ext_csds(struct mmcsd_card *card, uint8_t *ext_csd,
								uint32_t bus_width) {
	uint8_t *bw_ext_csd;
	int err;

	if (bus_width == MMCSD_BUS_WIDTH_1)
		return 0;

	err = mmc_get_ext_csd(card, &bw_ext_csd);
	if (err || bw_ext_csd == NULL) {
		err = -EINVAL;
		goto out;
	}

	/* only compare read only fields */
	err = !(
		(ext_csd[EXT_CSD_PARTITION_SUPPORT] == bw_ext_csd[EXT_CSD_PARTITION_SUPPORT]) &&
		(ext_csd[EXT_CSD_ERASED_MEM_CONT] == bw_ext_csd[EXT_CSD_ERASED_MEM_CONT]) &&
		(ext_csd[EXT_CSD_REV] == bw_ext_csd[EXT_CSD_REV]) &&
		(ext_csd[EXT_CSD_STRUCTURE] == bw_ext_csd[EXT_CSD_STRUCTURE]) &&
		(ext_csd[EXT_CSD_CARD_TYPE] == bw_ext_csd[EXT_CSD_CARD_TYPE]) &&
		(ext_csd[EXT_CSD_S_A_TIMEOUT] == bw_ext_csd[EXT_CSD_S_A_TIMEOUT]) &&
		(ext_csd[EXT_CSD_HC_WP_GRP_SIZE] == bw_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]) &&
		(ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT] == bw_ext_csd[EXT_CSD_ERASE_TIMEOUT_MULT]) &&
		(ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] == bw_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE]) &&
		(ext_csd[EXT_CSD_SEC_TRIM_MULT] == bw_ext_csd[EXT_CSD_SEC_TRIM_MULT]) &&
		(ext_csd[EXT_CSD_SEC_ERASE_MULT] == bw_ext_csd[EXT_CSD_SEC_ERASE_MULT]) &&
		(ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT] ==
		 bw_ext_csd[EXT_CSD_SEC_FEATURE_SUPPORT]) &&
		(ext_csd[EXT_CSD_TRIM_MULT] == bw_ext_csd[EXT_CSD_TRIM_MULT]) &&
		(ext_csd[EXT_CSD_SEC_CNT + 0] == bw_ext_csd[EXT_CSD_SEC_CNT + 0]) &&
		(ext_csd[EXT_CSD_SEC_CNT + 1] == bw_ext_csd[EXT_CSD_SEC_CNT + 1]) &&
		(ext_csd[EXT_CSD_SEC_CNT + 2] == bw_ext_csd[EXT_CSD_SEC_CNT + 2]) &&
		(ext_csd[EXT_CSD_SEC_CNT + 3] == bw_ext_csd[EXT_CSD_SEC_CNT + 3]) &&
		(ext_csd[EXT_CSD_PWR_CL_52_195] == bw_ext_csd[EXT_CSD_PWR_CL_52_195]) &&
		(ext_csd[EXT_CSD_PWR_CL_26_195] == bw_ext_csd[EXT_CSD_PWR_CL_26_195]) &&
		(ext_csd[EXT_CSD_PWR_CL_52_360] == bw_ext_csd[EXT_CSD_PWR_CL_52_360]) &&
		(ext_csd[EXT_CSD_PWR_CL_26_360] == bw_ext_csd[EXT_CSD_PWR_CL_26_360]) &&
		(ext_csd[EXT_CSD_PWR_CL_200_195] == bw_ext_csd[EXT_CSD_PWR_CL_200_195]) &&
		(ext_csd[EXT_CSD_PWR_CL_200_360] == bw_ext_csd[EXT_CSD_PWR_CL_200_360]) &&
		(ext_csd[EXT_CSD_PWR_CL_DDR_52_195] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_195]) &&
		(ext_csd[EXT_CSD_PWR_CL_DDR_52_360] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_52_360]) &&
		(ext_csd[EXT_CSD_PWR_CL_DDR_200_360] == bw_ext_csd[EXT_CSD_PWR_CL_DDR_200_360]));

	if (err)
		err = -EINVAL;

out:
	kfree(bw_ext_csd);
	return err;
}

/*
 * Select the bus width among 4-bit and 8-bit(SDR).
 * If the bus width is changed successfully, return the selected width value.
 * Zero is returned instead of error value if the wide width is not supported.
 */
static int mmc_select_bus_width(struct mmcsd_card *card, uint8_t *ext_csd) {
	uint32_t ext_csd_bits[][2] = {
		{EXT_CSD_BUS_WIDTH_8, EXT_CSD_DDR_BUS_WIDTH_8},
		{EXT_CSD_BUS_WIDTH_4, EXT_CSD_DDR_BUS_WIDTH_4},
		{EXT_CSD_BUS_WIDTH_1, EXT_CSD_BUS_WIDTH_1},
	};
	uint32_t bus_widths[] = {MMCSD_BUS_WIDTH_8, MMCSD_BUS_WIDTH_4, MMCSD_BUS_WIDTH_1};
	struct mmcsd_host *host = card->host;
	unsigned idx, bus_width = 0;
	int err = 0, ddr = 0;

	if (GET_BITS(card->resp_csd, 122, 4) < 4)
		return 0;

	if (card->flags & CARD_FLAG_HIGHSPEED_DDR) {
		ddr = 2;
	}
	/*
	 * Unlike SD, MMC cards don't have a configuration register to notify
	 * supported bus width. So bus test command should be run to identify
	 * the supported bus width or compare the EXT_CSD values of current
	 * bus width and EXT_CSD values of 1 bit mode read earlier.
	 */
	for (idx = 0; idx < sizeof(bus_widths) / sizeof(uint32_t); idx++) {
		/*
		 * Determine BUS WIDTH mode according to the capability of host
		 */
		if (((ext_csd_bits[idx][0] == EXT_CSD_BUS_WIDTH_8) &&
			 ((host->flags & MMCSD_BUSWIDTH_8) == 0)) ||
			((ext_csd_bits[idx][0] == EXT_CSD_BUS_WIDTH_4) &&
			 ((host->flags & MMCSD_BUSWIDTH_4) == 0))) {
			continue;
		}
		bus_width = bus_widths[idx];
		if (bus_width == MMCSD_BUS_WIDTH_1) {
			ddr = 0;
		}

		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
						 ext_csd_bits[idx][0]);
		if (err)
			continue;

		mmcsd_set_bus_width(host, bus_width);
		err = mmc_compare_ext_csds(card, ext_csd, bus_width);
		if (!err) {
			break;
		} else {
			switch (ext_csd_bits[idx][0]) {
			case 0:
				pr_err("switch to bus width 1 bit failed!");
				break;
			case 1:
				pr_err("switch to bus width 4 bit failed!");
				break;
			case 2:
				pr_err("switch to bus width 8 bit failed!");
				break;
			default:
				break;
			}
		}
	}

	if (!err && ddr) {
		err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH,
						 ext_csd_bits[idx][1]);
	}

	if (!err) {
		if (card->flags & (CARD_FLAG_HIGHSPEED | CARD_FLAG_HIGHSPEED_DDR)) {
			err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
		}
	}

	return err;
}

int mmc_send_op_cond(struct mmcsd_host *host, uint32_t ocr, uint32_t *rocr) {
	struct mmcsd_cmd cmd;
	uint32_t i;
	int err = 0;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SEND_OP_COND;
	cmd.arg = controller_is_spi(host) ? 0 : ocr;
	cmd.flags = RESP_SPI_R1 | RESP_R3 | CMD_BCR;

	for (i = 100; i; i--) {
		err = mmcsd_send_cmd(host, &cmd, 3);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (controller_is_spi(host)) {
			if (!(cmd.resp[0] & R1_SPI_IDLE))
				break;
		} else {
			if (cmd.resp[0] & CARD_BUSY)
				break;
		}

		err = -ETIMEDOUT;
		tx_thread_sleep(TX_MSEC(10)); // delay 10ms
	}

	if (rocr && !controller_is_spi(host))
		*rocr = cmd.resp[0];

	return err;
}

static int mmc_set_card_addr(struct mmcsd_host *host, uint32_t rca) {
	int err;
	struct mmcsd_cmd cmd;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SET_RELATIVE_ADDR;
	cmd.arg = rca << 16;
	cmd.flags = RESP_R1 | CMD_AC;
	err = mmcsd_send_cmd(host, &cmd, 3);
	if (err)
		return err;

	return 0;
}

static int mmc_select_hs200(struct mmcsd_card *card) {
	int ret;

	ret =
		mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS200);
	if (ret)
		return ret;

	mmcsd_set_timing(card->host, MMCSD_TIMING_MMC_HS200);
	mmcsd_set_clock(card->host, card->max_data_rate);
	return mmcsd_excute_tuning(card);
}

static int mmc_switch_to_hs400(struct mmcsd_card *card) {
	struct mmcsd_host *host = card->host;
	int err;
	uint8_t ext_csd_bus_width;
	uint32_t hs_timing;

	/* Switch to HS_TIMING to 0x01 (High Speed) */
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS);
	if (err)
		return err;

	mmcsd_set_timing(card->host, MMCSD_TIMING_MMC_HS);
	/* Host changes frequency to <= 52MHz  */
	mmcsd_set_clock(card->host, 52000000);

	bool suppoenhanced_ds = ((card->ext_csd.enhanced_data_strobe != 0) &&
							 ((host->flags & MMCSD_SUP_ENH_DS) != 0));

	/* Set the bus width to:
	 *  0x86 if enhanced data strobe is supported, or
	 *  0x06 if enhanced data strobe is not supported
	 */
	ext_csd_bus_width =
		suppoenhanced_ds ? EXT_CSD_DDR_BUS_WIDTH_8_EH_DS : EXT_CSD_DDR_BUS_WIDTH_8;
	err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_BUS_WIDTH, ext_csd_bus_width);
	if (err != 0) {
		return err;
	}

	/* Set HS_TIMING to 0x03 (HS400) */
	err =
		mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, EXT_CSD_TIMING_HS400);
	if (err != 0) {
		return err;
	}

	/* Change the Host timing accordingly */
	hs_timing = suppoenhanced_ds ? MMCSD_TIMING_MMC_HS400_ENH_DS : MMCSD_TIMING_MMC_HS400;
	mmcsd_set_timing(host, hs_timing);

	/* Host may changes frequency to <= 200MHz */
	mmcsd_set_clock(card->host, card->max_data_rate);

	return 0;
}

static int mmc_select_hs400(struct mmcsd_card *card) {
	int ret;
	struct mmcsd_host *host = card->host;
	/* if the card or host doesn't support enhanced data strobe, switch to HS200 and
	 * perform tuning process first */
	if ((card->ext_csd.enhanced_data_strobe == 0) ||
		((host->flags & MMCSD_SUP_ENH_DS) == 0)) {
		ret = mmc_select_hs200(card);
		if (ret != 0) {
			return ret;
		}
	}
	return mmc_switch_to_hs400(card);
}

static int mmc_select_timing(struct mmcsd_card *card) {
	int ret = 0;

	if (card->flags & CARD_FLAG_HS400) {
		pr_info("switch to HS400 mode");
		ret = mmc_select_hs400(card);
	} else if (card->flags & CARD_FLAG_HS200) {
		pr_info("switch to HS200 mode");
		ret = mmc_select_hs200(card);
	} else if (card->flags & CARD_FLAG_HIGHSPEED_DDR) {
		pr_info("switch to HIGH Speed DDR mode");
		mmcsd_set_timing(card->host, MMCSD_TIMING_MMC_DDR52);
		mmcsd_set_clock(card->host, card->hs_max_data_rate);
	} else {
		pr_info("switch to HIGH Speed mode");
		mmcsd_set_timing(card->host, MMCSD_TIMING_MMC_HS);
		mmcsd_set_clock(card->host, card->hs_max_data_rate);
	}

	return ret;
}

static int32_t mmcsd_mmc_init_card(struct mmcsd_host *host, uint32_t ocr) {
	int32_t err;
	uint32_t resp[4];
	uint32_t rocr = 0;
	uint8_t *ext_csd = NULL;
	struct mmcsd_card *card = NULL;

	mmcsd_go_idle(host);

	/* The extra bit indicates that we support high capacity */
	err = mmc_send_op_cond(host, ocr | (1 << 30), &rocr);
	if (err)
		goto err;

	if (controller_is_spi(host)) {
		err = mmcsd_spi_use_crc(host, 1);
		if (err)
			goto err1;
	}

	if (controller_is_spi(host))
		err = mmcsd_get_cid(host, resp);
	else
		err = mmcsd_all_get_cid(host, resp);
	if (err)
		goto err;

	card = kmalloc(sizeof(struct mmcsd_card), GMF_KERNEL);
	if (!card) {
		pr_err("malloc card failed!");
		err = -ENOMEM;
		goto err;
	}
	memset(card, 0, sizeof(struct mmcsd_card));

	card->card_type = CARD_TYPE_MMC;
	card->host = host;
	card->rca = 1;
	memcpy(card->resp_cid, resp, sizeof(card->resp_cid));

	/*
	 * For native busses:  get card RCA and quit open drain mode.
	 */
	if (!controller_is_spi(host)) {
		err = mmc_set_card_addr(host, card->rca);
		if (err)
			goto err1;

		mmcsd_set_bus_mode(host, MMCSD_BUSMODE_PUSHPULL);
	}

	err = mmcsd_get_csd(card, card->resp_csd);
	if (err)
		goto err1;

	err = mmcsd_parse_csd(card);
	if (err)
		goto err1;

	if (!controller_is_spi(host)) {
		err = mmcsd_select_card(card);
		if (err)
			goto err1;
	}

	/*
	 * Fetch and process extended CSD.
	 */

	err = mmc_get_ext_csd(card, &ext_csd);
	if (err)
		goto err1;
	err = mmc_parse_ext_csd(card, ext_csd);
	if (err)
		goto err1;

	/* If doing byte addressing, check if required to do sector
	 * addressing.  Handle the case of <2GB cards needing sector
	 * addressing.  See section 8.1 JEDEC Standard JED84-A441;
	 * ocr register has bit 30 set for sector addressing.
	 */
	if (!(card->flags & CARD_FLAG_SDHC) && (rocr & (1 << 30)))
		card->flags |= CARD_FLAG_SDHC;

	/*switch bus width and bus mode*/
	err = mmc_select_bus_width(card, ext_csd);
	if (err) {
		pr_err("mmc select buswidth fail");
		goto err0;
	}

	err = mmc_select_timing(card);
	if (err) {
		pr_err("mmc select timing fail");
		goto err0;
	}

	if (card->ext_csd.cache_size > 0) {
		mmc_switch(card, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_CACHE_CTRL, 1);
	}

	host->card = card;
	kfree(ext_csd);
	return 0;

err0:
	kfree(ext_csd);
err1:
	kfree(card);
err:
	return err;
}

/*
 * Starting point for mmc card init.
 */
int init_mmc(struct mmcsd_host *host, uint32_t ocr) {
	int32_t err;
	uint32_t current_ocr;
	/*
	 * We need to get OCR a different way for SPI.
	 */
	if (controller_is_spi(host)) {
		err = mmcsd_spi_read_ocr(host, 0, &ocr);
		if (err)
			goto err;
	}

	current_ocr = mmcsd_select_voltage(host, ocr);

	/*
	 * Can we support the voltage(s) of the card(s)?
	 */
	if (!current_ocr) {
		err = -EINVAL;
		goto err;
	}

	/*
	 * Detect and init the card.
	 */
	err = mmcsd_mmc_init_card(host, current_ocr);
	if (err)
		goto err;

	mmcsd_host_unlock(host);
	err = mmcsd_blkdev_probe(host->card);
	if (err)
		goto remove_card;
	mmcsd_host_lock(host);
	return 0;

remove_card:
	mmcsd_host_lock(host);
	mmcsd_blkdev_remove(host->card);
	kfree(host->card);
	host->card = NULL;
err:
	pr_err("init MMC card failed!");
	return err;
}
