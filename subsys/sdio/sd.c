/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 * 2024-05-26     HPMicro       add UHS-I support
 */

#define pr_fmt(fmt) "[SDIO]: " fmt "\n"

#include <errno.h>

#include <tx_api.h>
#include <basework/log.h>
#include <basework/byteorder.h>

#include <drivers/sdio/mmcsd_core.h>
#include <drivers/sdio/sd.h>

#define be32_to_cpu(x)  rte_be32_to_cpu(x)

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

static inline uint32_t 
GET_BITS(uint32_t *resp, uint32_t start, uint32_t size) {
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

static int32_t mmcsd_parse_csd(struct mmcsd_card *card) {
	struct mmcsd_csd *csd = &card->csd;
	uint32_t *resp = card->resp_csd;

	csd->csd_structure = GET_BITS(resp, 126, 2);

	switch (csd->csd_structure) {
	case 0:
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
		card->card_capacity = (csd->c_size + 1) << (csd->c_size_mult + 2);
		card->card_capacity *= card->card_blksize;
		card->card_capacity >>= 10; /* unit:KB */
		card->tacc_clks = csd->nsac * 100;
		card->tacc_ns =
			(tacc_uint[csd->taac & 0x07] * tacc_value[(csd->taac & 0x78) >> 3] + 9) / 10;
		card->max_data_rate =
			tran_unit[csd->tran_speed & 0x07] * tran_value[(csd->tran_speed & 0x78) >> 3];

		break;
	case 1:
		card->flags |= CARD_FLAG_SDHC;

		/*This field is fixed to 0Eh, which indicates 1 ms.
		  The host should not use TAAC, NSAC, and R2W_FACTOR
		  to calculate timeout and should uses fixed timeout
		  values for read and write operations*/
		csd->taac = GET_BITS(resp, 112, 8);
		csd->nsac = GET_BITS(resp, 104, 8);
		csd->tran_speed = GET_BITS(resp, 96, 8);
		csd->card_cmd_class = GET_BITS(resp, 84, 12);
		csd->rd_blk_len = GET_BITS(resp, 80, 4);
		csd->rd_blk_part = GET_BITS(resp, 79, 1);
		csd->wr_blk_misalign = GET_BITS(resp, 78, 1);
		csd->rd_blk_misalign = GET_BITS(resp, 77, 1);
		csd->dsr_imp = GET_BITS(resp, 76, 1);
		csd->c_size = GET_BITS(resp, 48, 22);

		csd->r2w_factor = GET_BITS(resp, 26, 3);
		csd->wr_blk_len = GET_BITS(resp, 22, 4);
		csd->wr_blk_partial = GET_BITS(resp, 21, 1);
		csd->csd_crc = GET_BITS(resp, 1, 7);

		card->card_blksize = 512;
		card->card_capacity = (csd->c_size + 1) * 512; /* unit:KB */
		card->card_sec_cnt = card->card_capacity * 2;
		card->tacc_clks = 0;
		card->tacc_ns = 0;
		card->max_data_rate =
			tran_unit[csd->tran_speed & 0x07] * tran_value[(csd->tran_speed & 0x78) >> 3];

		break;
	default:
		pr_err("unrecognised CSD structure version %d!", csd->csd_structure);
		return -EINVAL;
	}

	pr_info("SD card capacity %d KB.", card->card_capacity);
	return 0;
}

static int32_t mmcsd_parse_scr(struct mmcsd_card *card) {
	struct sd_scr *scr = &card->scr;
	uint32_t resp[4];

	resp[3] = card->resp_scr[1];
	resp[2] = card->resp_scr[0];
	scr->sd_version = GET_BITS(resp, 56, 4);
	scr->sd_bus_widths = GET_BITS(resp, 48, 4);

	return 0;
}

static int32_t mmcsd_switch(struct mmcsd_card *card) {
	int32_t err;
	struct mmcsd_host *host = card->host;
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;
	uint32_t stackbuf[16];
	uint8_t *buf = (uint8_t *)stackbuf;

	if (card->card_type != CARD_TYPE_SD)
		goto err;
	if (card->scr.sd_version < SCR_SPEC_VER_1)
		goto err;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_SWITCH;
	cmd.arg = 0x00FFFFF1;
	cmd.flags = RESP_R1 | CMD_ADTC;

	memset(&data, 0, sizeof(struct mmcsd_data));
	mmcsd_set_data_timeout(&data, card);
	data.blksize = 64;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = (uint32_t *)buf;

	memset(&req, 0, sizeof(struct mmcsd_req));
	req.cmd = &cmd;
	req.data = &data;

	mmcsd_send_request(host, &req);
	if (cmd.err || data.err) {
		goto err1;
	}

	if (buf[13] & 0x02)
		card->hs_max_data_rate = 50000000;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_SWITCH;

	uint32_t switch_func_timing;
	if ((card->flags & CARD_FLAG_SDR104) && (card->host->flags & MMCSD_SUP_SDR104)) {
		switch_func_timing = SD_SWITCH_FUNC_TIMING_SDR104;
	} else if ((card->flags & CARD_FLAG_SDR50) && (card->host->flags & MMCSD_SUP_SDR50)) {
		switch_func_timing = SD_SWITCH_FUNC_TIMING_SDR50;
	} else if ((card->flags & CARD_FLAG_DDR50) && (card->host->flags & MMCSD_SUP_DDR50)) {
		switch_func_timing = SD_SWITCH_FUNC_TIMING_DDR50;
	} else {
		switch_func_timing = SD_SWITCH_FUNC_TIMING_HS;
	}
	cmd.arg = 0x80FFFFF0 | switch_func_timing;
	cmd.flags = RESP_R1 | CMD_ADTC;

	memset(&data, 0, sizeof(struct mmcsd_data));
	mmcsd_set_data_timeout(&data, card);

	data.blksize = 64;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = (uint32_t *)buf;
	memset(&req, 0, sizeof(struct mmcsd_req));

	req.cmd = &cmd;
	req.data = &data;
	mmcsd_send_request(host, &req);

	if (cmd.err || data.err) {
		goto err1;
	}

	if ((buf[16] & 0xF) != switch_func_timing) {
		pr_err("switching card to timing mode %d failed!", switch_func_timing);
		goto err;
	}

	switch (switch_func_timing) {
	case SD_SWITCH_FUNC_TIMING_SDR104:
		card->flags |= CARD_FLAG_SDR104;
		break;
	case SD_SWITCH_FUNC_TIMING_SDR50:
		card->flags |= CARD_FLAG_SDR50;
		break;
	case SD_SWITCH_FUNC_TIMING_DDR50:
		card->flags |= CARD_FLAG_DDR50;
		break;
	case SD_SWITCH_FUNC_TIMING_HS:
		card->flags |= CARD_FLAG_HIGHSPEED;
		break;
	default:
		/* Default speed */
		break;
	}

	card->max_data_rate = 50000000;
	if (switch_func_timing == SD_SWITCH_FUNC_TIMING_SDR104) {
		pr_info("switch to SDR104 mode");
		mmcsd_set_timing(card->host, MMCSD_TIMING_UHS_SDR104);
		mmcsd_set_clock(card->host, 208000000);
		err = mmcsd_excute_tuning(card);
		card->max_data_rate = 208000000;
	} else if (switch_func_timing == SD_SWITCH_FUNC_TIMING_SDR50) {
		pr_info("switch to SDR50 mode");
		mmcsd_set_timing(card->host, MMCSD_TIMING_UHS_SDR50);
		mmcsd_set_clock(card->host, 100000000);
		err = mmcsd_excute_tuning(card);
		card->max_data_rate = 10000000;
	} else if (switch_func_timing == SD_SWITCH_FUNC_TIMING_DDR50) {
		pr_info("switch to DDR50 mode");
		mmcsd_set_timing(card->host, MMCSD_TIMING_UHS_DDR50);
		mmcsd_set_clock(card->host, 50000000);
	} else {
		pr_info("switch to High Speed / SDR25 mode ");
		mmcsd_set_timing(card->host, MMCSD_TIMING_SD_HS);
		mmcsd_set_clock(card->host, 50000000);
	}

err:
	return 0;
err1:
	if (cmd.err)
		err = cmd.err;
	if (data.err)
		err = data.err;
	return err;
}

static int mmcsd_app_cmd(struct mmcsd_host *host, struct mmcsd_card *card) {
	int err;
	struct mmcsd_cmd cmd = {0};

	cmd.cmd_code = APP_CMD;
	if (card) {
		cmd.arg = card->rca << 16;
		cmd.flags = RESP_R1 | CMD_AC;
	} else {
		cmd.arg = 0;
		cmd.flags = RESP_R1 | CMD_BCR;
	}

	err = mmcsd_send_cmd(host, &cmd, 0);
	if (err)
		return err;

	/* Check that card supported application commands */
	if (!controller_is_spi(host) && !(cmd.resp[0] & R1_APP_CMD))
		return -EINVAL;

	return 0;
}

int mmcsd_send_app_cmd(struct mmcsd_host *host, struct mmcsd_card *card,
					   struct mmcsd_cmd *cmd, int retry) {
	struct mmcsd_req req;
	int i;
	int err;

	err = -EINVAL;

	/*
	 * We have to resend MMC_APP_CMD for each attempt so
	 * we cannot use the retries field in mmc_command.
	 */
	for (i = 0; i <= retry; i++) {
		memset(&req, 0, sizeof(struct mmcsd_req));
		err = mmcsd_app_cmd(host, card);
		if (err) {
			/* no point in retrying; no APP commands allowed */
			if (controller_is_spi(host)) {
				if (cmd->resp[0] & R1_SPI_ILLEGAL_COMMAND)
					break;
			}
			continue;
		}

		memset(&req, 0, sizeof(struct mmcsd_req));
		memset(cmd->resp, 0, sizeof(cmd->resp));
		req.cmd = cmd;
		// cmd->data = NULL;
		mmcsd_send_request(host, &req);
		err = cmd->err;
		if (!cmd->err)
			break;

		/* no point in retrying illegal APP commands */
		if (controller_is_spi(host)) {
			if (cmd->resp[0] & R1_SPI_ILLEGAL_COMMAND)
				break;
		}
	}

	return err;
}

int mmcsd_app_set_bus_width(struct mmcsd_card *card, int32_t width) {
	int err;
	struct mmcsd_cmd cmd;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_APP_SET_BUS_WIDTH;
	cmd.flags = RESP_R1 | CMD_AC;

	switch (width) {
	case MMCSD_BUS_WIDTH_1:
		cmd.arg = MMCSD_BUS_WIDTH_1;
		break;
	case MMCSD_BUS_WIDTH_4:
		cmd.arg = MMCSD_BUS_WIDTH_4;
		break;
	default:
		return -EINVAL;
	}

	err = mmcsd_send_app_cmd(card->host, card, &cmd, 3);
	if (err)
		return err;

	return 0;
}

int mmcsd_send_app_op_cond(struct mmcsd_host *host, uint32_t ocr, uint32_t *rocr) {
	struct mmcsd_cmd cmd;
	uint32_t i;
	int err = 0;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_APP_OP_COND;
	if (controller_is_spi(host))
		cmd.arg = ocr & (1 << 30); /* SPI only defines one bit */
	else
		cmd.arg = ocr;
	cmd.flags = RESP_SPI_R1 | RESP_R3 | CMD_BCR;

	for (i = 1000; i; i--) {
		err = mmcsd_send_app_cmd(host, NULL, &cmd, 3);
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

/*
 * To support SD 2.0 cards, we must always invoke SD_SEND_IF_COND
 * before SD_APP_OP_COND. This command will harmlessly fail for
 * SD 1.0 cards.
 */
int mmcsd_send_if_cond(struct mmcsd_host *host, uint32_t ocr) {
	struct mmcsd_cmd cmd;
	int err;
	uint8_t pattern;

	cmd.cmd_code = SD_SEND_IF_COND;
	cmd.arg = ((ocr & 0xFF8000) != 0) << 8 | 0xAA;
	cmd.flags = RESP_SPI_R7 | RESP_R7 | CMD_BCR;
	err = mmcsd_send_cmd(host, &cmd, 0);
	if (err)
		return err;

	if (controller_is_spi(host))
		pattern = cmd.resp[1] & 0xFF;
	else
		pattern = cmd.resp[0] & 0xFF;

	if (pattern != 0xAA)
		return -EINVAL;

	return 0;
}

int mmcsd_get_card_addr(struct mmcsd_host *host, uint32_t *rca) {
	int err;
	struct mmcsd_cmd cmd;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	cmd.flags = RESP_R6 | CMD_BCR;

	err = mmcsd_send_cmd(host, &cmd, 3);
	if (err)
		return err;

	*rca = cmd.resp[0] >> 16;
	return 0;
}

int mmcsd_get_scr(struct mmcsd_card *card, uint32_t *scr) {
	int32_t err;
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;

	err = mmcsd_app_cmd(card->host, card);
	if (err)
		return err;

	memset(&req, 0, sizeof(struct mmcsd_req));
	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	memset(&data, 0, sizeof(struct mmcsd_data));

	req.cmd = &cmd;
	req.data = &data;
	cmd.cmd_code = SD_APP_SEND_SCR;
	cmd.arg = 0;
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
	data.blksize = 8;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = scr;

	mmcsd_set_data_timeout(&data, card);
	mmcsd_send_request(card->host, &req);
	if (cmd.err)
		return cmd.err;
	if (data.err)
		return data.err;

	scr[0] = be32_to_cpu(scr[0]);
	scr[1] = be32_to_cpu(scr[1]);

	return 0;
}

static int mmcsd_read_sd_status(struct mmcsd_card *card, uint32_t *sd_status) {
	int32_t err;
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;

	err = mmcsd_app_cmd(card->host, card);
	if (err)
		return err;

	memset(&req, 0, sizeof(struct mmcsd_req));
	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	memset(&data, 0, sizeof(struct mmcsd_data));
	req.cmd = &cmd;
	req.data = &data;
	cmd.cmd_code = SEND_STATUS;
	cmd.arg = 0;
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
	data.blksize = 64;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = sd_status;

	mmcsd_set_data_timeout(&data, card);
	mmcsd_send_request(card->host, &req);
	if (cmd.err)
		return cmd.err;
	if (data.err)
		return data.err;

	/* Convert endian */
	for (uint32_t i = 0; i < 8; i++) {
		uint32_t tmp = sd_status[i];
		sd_status[i] = sd_status[15 - i];
		sd_status[15 - i] = tmp;
	}
	for (uint32_t i = 0; i < 16; i++) {
		sd_status[i] = be32_to_cpu(sd_status[i]);
	}

	return 0;
}

static int sd_switch_voltage(struct mmcsd_host *host) {
	int err;
	struct mmcsd_cmd cmd = {0};

	cmd.cmd_code = VOLTAGE_SWITCH;
	cmd.arg = 0;
	cmd.flags = RESP_R1 | CMD_AC;
	err = mmcsd_send_cmd(host, &cmd, 0);
	if (err)
		return err;

	return 0;
}

static int sd_switch_uhs_voltage(struct mmcsd_host *host) {
	if (host->ops.switch_uhs_voltage != NULL) {
		return MMCSD_SWITCH_UHS_VOLTAGE(host);
	}
	return -ENOSYS;
}

static int32_t mmcsd_sd_init_card(struct mmcsd_host *host, uint32_t ocr) {
	struct mmcsd_card *card;
	int32_t err;
	uint32_t resp[4];
	uint32_t max_data_rate;

	mmcsd_go_idle(host);

	/*
	 * If SD_SEND_IF_COND indicates an SD 2.0
	 * compliant card and we should set bit 30
	 * of the ocr to indicate that we can handle
	 * block-addressed SDHC cards.
	 */
	err = mmcsd_send_if_cond(host, ocr);
	if (!err)
		ocr |= 1 << 30;

	/* Switch to UHS voltage if both Host and the Card support this feature */
	if (((host->valid_ocr & VDD_165_195) != 0) &&
		(host->ops.switch_uhs_voltage != NULL)) {
		ocr |= OCR_S18R;
	}
	err = mmcsd_send_app_op_cond(host, ocr, &ocr);
	if (err)
		goto err2;

	/* Select voltage */
	if (ocr & OCR_S18R) {
		ocr = VDD_165_195;
		err = sd_switch_voltage(host);
		if (err)
			goto err2;
		err = sd_switch_uhs_voltage(host);
		if (err)
			goto err2;
	}

	if (controller_is_spi(host))
		err = mmcsd_get_cid(host, resp);
	else
		err = mmcsd_all_get_cid(host, resp);
	if (err)
		goto err2;

	card = kmalloc(sizeof(struct mmcsd_card), GMF_KERNEL);
	if (!card) {
		pr_err("malloc card failed!");
		err = -ENOMEM;
		goto err2;
	}
	memset(card, 0, sizeof(struct mmcsd_card));
	card->card_type = CARD_TYPE_SD;
	card->host = host;
	memcpy(card->resp_cid, resp, sizeof(card->resp_cid));

	/*
	 * For native busses:  get card RCA and quit open drain mode.
	 */
	if (!controller_is_spi(host)) {
		err = mmcsd_get_card_addr(host, &card->rca);
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

	err = mmcsd_get_scr(card, card->resp_scr);
	if (err)
		goto err1;

	mmcsd_parse_scr(card);

	if (controller_is_spi(host)) {
		err = mmcsd_spi_use_crc(host, 1);
		if (err)
			goto err1;
	}

	mmcsd_set_timing(host, MMCSD_TIMING_LEGACY);
	mmcsd_set_clock(host, 25000000);
	/*switch bus width*/
	if ((host->flags & MMCSD_BUSWIDTH_4) &&
		(card->scr.sd_bus_widths & SD_SCR_BUS_WIDTH_4)) {
		err = mmcsd_app_set_bus_width(card, MMCSD_BUS_WIDTH_4);
		if (err)
			goto err1;

		mmcsd_set_bus_width(host, MMCSD_BUS_WIDTH_4);
	}

	/* Read and decode SD Status and check whether UHS mode is supported */
	union sd_status sd_status;
	err = mmcsd_read_sd_status(card, sd_status.status_words);
	if (err)
		goto err1;
	if ((sd_status.uhs_speed_grade > 0) && (ocr & VDD_165_195)) {
		/* Assume the card supports all UHS-I modes because we cannot find any
		 * mainstreaming card that can support only part of the following modes.
		 */
		card->flags |= CARD_FLAG_SDR50 | CARD_FLAG_SDR104 | CARD_FLAG_DDR50;
	}

	/*
	 * change SD card to the highest supported speed
	 */
	err = mmcsd_switch(card);
	if (err)
		goto err1;

	/* set bus speed */
	max_data_rate = 0U;
	if (max_data_rate < card->hs_max_data_rate) {
		max_data_rate = card->hs_max_data_rate;
	}
	if (max_data_rate < card->max_data_rate) {
		max_data_rate = card->max_data_rate;
	}

	mmcsd_set_clock(host, max_data_rate);
	host->card = card;
	return 0;

err1:
	kfree(card);
err2:
	return err;
}

/*
 * Starting point for SD card init.
 */
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
int init_sd(struct mmcsd_host *host, uint32_t ocr) {
	int32_t err = -EINVAL;
	uint32_t current_ocr;
	/*
	 * We need to get OCR a different way for SPI.
	 */
	if (controller_is_spi(host)) {
		mmcsd_go_idle(host);
		err = mmcsd_spi_read_ocr(host, 0, &ocr);
		if (err)
			goto _err;
	}

	current_ocr = mmcsd_select_voltage(host, ocr);

	/*
	 * Can we support the voltage(s) of the card(s)?
	 */
	if (!current_ocr) {
		err = -EINVAL;
		goto _err;
	}

	/*
	 * Detect and init the card.
	 */
	err = mmcsd_sd_init_card(host, current_ocr);
	if (err)
		goto _err;

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
_err:

	pr_err("init SD card failed!");
	return err;
}
