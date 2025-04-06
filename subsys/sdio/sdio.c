/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-01-13     weety         first version
 * 2024-07-04     Evlers        fix an issue where repeated remove of card resulted in
 * assertions 2024-07-05     Evlers        fix a bug that read members in non-existent
 * functions
 */

#define pr_fmt(fmt) "[SDIO]: " fmt

#include <errno.h>
#include <tx_api.h>
#include <tx_timer.h>

#include <base/assert.h>
#include <base/container/list.h>
#include <base/log.h>

#include <drivers/sdio/mmcsd_core.h>
#include <drivers/sdio/sd.h>
#include <drivers/sdio/sdio.h>

#ifndef CONFIG_SDIO_STACK_SIZE
#define CONFIG_SDIO_STACK_SIZE 1700
#endif
#ifndef CONFIG_SDIO_THREAD_PRIORITY
#define CONFIG_SDIO_THREAD_PRIORITY 28
#endif

#ifndef MIN
#define MIN(a, b) rte_min(a, b)
#endif

struct sdio_card {
	struct mmcsd_card *card;
	struct rte_list list;
};

struct sdio_private {
	struct sdio_driver *drv;
	struct rte_list list;
};

static ULONG sdio_thread_stack[CONFIG_SDIO_STACK_SIZE / sizeof(ULONG)];
static struct rte_list sdio_cards = RTE_LIST_INIT(sdio_cards);
static struct rte_list sdio_drivers = RTE_LIST_INIT(sdio_drivers);
static const uint8_t speed_value[16] = {0,	10, 12, 13, 15, 20, 25, 30,
										35, 40, 45, 50, 55, 60, 70, 80};

static const uint32_t speed_unit[8] = {10000, 100000, 1000000, 10000000, 0, 0, 0, 0};

static inline int32_t sdio_match_card(struct mmcsd_card *card,
									  const struct sdio_device_id *id);

int32_t sdio_io_send_op_cond(struct mmcsd_host *host, uint32_t ocr, uint32_t *cmd5_resp) {
	struct mmcsd_cmd cmd;
	int32_t i, err = 0;

	rte_assert(host != NULL);
	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_IO_SEND_OP_COND;
	cmd.arg = ocr;
	cmd.flags = RESP_SPI_R4 | RESP_R4 | CMD_BCR;

	for (i = 100; i; i--) {
		err = mmcsd_send_cmd(host, &cmd, 0);
		if (err)
			break;

		/* if we're just probing, do a single pass */
		if (ocr == 0)
			break;

		/* otherwise wait until reset completes */
		if (controller_is_spi(host)) {
			/*
			 * Both R1_SPI_IDLE and MMC_CARD_BUSY indicate
			 * an initialized card under SPI, but some cards
			 * (Marvell's) only behave when looking at this
			 * one.
			 */
			if (cmd.resp[1] & CARD_BUSY)
				break;
		} else {
			if (cmd.resp[0] & CARD_BUSY)
				break;
		}

		err = -ETIMEDOUT;
		tx_thread_sleep(TX_MSEC(10));
	}

	if (cmd5_resp)
		*cmd5_resp = cmd.resp[controller_is_spi(host) ? 1 : 0];

	return err;
}

int32_t sdio_io_rw_direct(struct mmcsd_card *card, int32_t rw, uint32_t fn,
						  uint32_t reg_addr, uint8_t *pdata, uint8_t raw) {
	struct mmcsd_cmd cmd;
	int32_t err;

	rte_assert(card != NULL);
	rte_assert(fn <= SDIO_MAX_FUNCTIONS);
	rte_assert(pdata != NULL);

	if (reg_addr & ~SDIO_ARG_CMD53_REG_MASK)
		return -EINVAL;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_IO_RW_DIRECT;
	cmd.arg = rw ? SDIO_ARG_CMD52_WRITE : SDIO_ARG_CMD52_READ;
	cmd.arg |= fn << SDIO_ARG_CMD52_FUNC_SHIFT;
	cmd.arg |= raw ? SDIO_ARG_CMD52_RAW_FLAG : 0x00000000;
	cmd.arg |= reg_addr << SDIO_ARG_CMD52_REG_SHIFT;
	cmd.arg |= *pdata;
	cmd.flags = RESP_SPI_R5 | RESP_R5 | CMD_AC;

	err = mmcsd_send_cmd(card->host, &cmd, 0);
	if (err)
		return err;

	if (!controller_is_spi(card->host)) {
		if (cmd.resp[0] & R5_ERROR)
			return -EIO;
		if (cmd.resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (cmd.resp[0] & R5_OUT_OF_RANGE)
			return -EINVAL;
	}

	if (!rw || raw) {
		if (controller_is_spi(card->host))
			*pdata = (cmd.resp[0] >> 8) & 0xFF;
		else
			*pdata = cmd.resp[0] & 0xFF;
	}

	return 0;
}

int32_t sdio_io_rw_extended(struct mmcsd_card *card, int32_t rw, uint32_t fn,
							uint32_t addr, int32_t op_code, uint8_t *buf, uint32_t blocks,
							uint32_t blksize) {
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;

	rte_assert(card != NULL);
	rte_assert(fn <= SDIO_MAX_FUNCTIONS);
	rte_assert(blocks != 1 || blksize <= 512);
	rte_assert(blocks != 0);
	rte_assert(blksize != 0);

	if (addr & ~SDIO_ARG_CMD53_REG_MASK)
		return -EINVAL;

	memset(&req, 0, sizeof(struct mmcsd_req));
	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	memset(&data, 0, sizeof(struct mmcsd_data));
	req.cmd = &cmd;
	req.data = &data;

	cmd.cmd_code = SD_IO_RW_EXTENDED;
	cmd.arg = rw ? SDIO_ARG_CMD53_WRITE : SDIO_ARG_CMD53_READ;
	cmd.arg |= fn << SDIO_ARG_CMD53_FUNC_SHIFT;
	cmd.arg |= op_code ? SDIO_ARG_CMD53_INCREMENT : 0x00000000;
	cmd.arg |= addr << SDIO_ARG_CMD53_REG_SHIFT;
	if (blocks == 1 && blksize <= 512)
		cmd.arg |= (blksize == 512) ? 0 : blksize; /* byte mode */
	else
		cmd.arg |= SDIO_ARG_CMD53_BLOCK_MODE | blocks; /* block mode */
	cmd.flags = RESP_SPI_R5 | RESP_R5 | CMD_ADTC;

	data.blksize = blksize;
	data.blks = blocks;
	data.flags = rw ? DATA_DIR_WRITE : DATA_DIR_READ;
	data.buf = (uint32_t *)buf;

	mmcsd_set_data_timeout(&data, card);
	mmcsd_send_request(card->host, &req);
	if (cmd.err)
		return cmd.err;
	if (data.err)
		return data.err;

	if (!controller_is_spi(card->host)) {
		if (cmd.resp[0] & R5_ERROR)
			return -EIO;
		if (cmd.resp[0] & R5_FUNCTION_NUMBER)
			return -EINVAL;
		if (cmd.resp[0] & R5_OUT_OF_RANGE)
			return -EINVAL;
	}

	return 0;
}

static inline uint32_t sdio_max_block_size(struct sdio_function *func) {
	uint32_t size = MIN(func->card->host->max_seg_size, func->card->host->max_blk_size);
	size = MIN(size, func->max_blk_size);
	return MIN(size, (uint32_t)512u); /* maximum size for byte mode */
}

int32_t sdio_io_rw_extended_block(struct sdio_function *func, int32_t rw, uint32_t addr,
								  int32_t op_code, uint8_t *buf, uint32_t len) {
	int32_t ret;
	uint32_t left_size;
	uint32_t max_blks, blks;

	rte_assert(func != NULL);
	rte_assert(func->card != NULL);
	left_size = len;

	/* Do the bulk of the transfer using block mode (if supported). */
	if (func->card->cccr.multi_block && (len > sdio_max_block_size(func))) {
		max_blks = MIN(func->card->host->max_blk_count,
					   func->card->host->max_seg_size / func->cur_blk_size);
		max_blks = MIN(max_blks, (uint32_t)511u);

		while (left_size > func->cur_blk_size) {
			blks = left_size / func->cur_blk_size;
			if (blks > max_blks)
				blks = max_blks;
			len = blks * func->cur_blk_size;

			ret = sdio_io_rw_extended(func->card, rw, func->num, addr, op_code, buf, blks,
									  func->cur_blk_size);
			if (ret)
				return ret;

			left_size -= len;
			buf += len;
			if (op_code)
				addr += len;
		}
	}

	while (left_size > 0) {
		len = MIN(left_size, sdio_max_block_size(func));
		ret = sdio_io_rw_extended(func->card, rw, func->num, addr, op_code, buf, 1, len);
		if (ret)
			return ret;

		left_size -= len;
		buf += len;
		if (op_code)
			addr += len;
	}

	return 0;
}

uint8_t sdio_io_readb(struct sdio_function *func, uint32_t reg, int32_t *err) {
	uint8_t data = 0;
	int32_t ret;

	ret = sdio_io_rw_direct(func->card, 0, func->num, reg, &data, 0);
	if (err) {
		*err = ret;
	}

	return data;
}

int32_t sdio_io_writeb(struct sdio_function *func, uint32_t reg, uint8_t data) {
	return sdio_io_rw_direct(func->card, 1, func->num, reg, &data, 0);
}

uint16_t sdio_io_readw(struct sdio_function *func, uint32_t addr, int32_t *err) {
	int32_t ret;
	uint32_t dmabuf;

	if (err)
		*err = 0;

	ret = sdio_io_rw_extended_block(func, 0, addr, 1, (uint8_t *)&dmabuf, 2);
	if (ret) {
		if (err)
			*err = ret;
	}

	return (uint16_t)dmabuf;
}

int32_t sdio_io_writew(struct sdio_function *func, uint16_t data, uint32_t addr) {
	uint32_t dmabuf = data;
	return sdio_io_rw_extended_block(func, 1, addr, 1, (uint8_t *)&dmabuf, 2);
}

uint32_t sdio_io_readl(struct sdio_function *func, uint32_t addr, int32_t *err) {
	int32_t ret;
	uint32_t dmabuf;

	if (err)
		*err = 0;

	ret = sdio_io_rw_extended_block(func, 0, addr, 1, (uint8_t *)&dmabuf, 4);
	if (ret) {
		if (err)
			*err = ret;
	}

	return dmabuf;
}

int32_t sdio_io_writel(struct sdio_function *func, uint32_t data, uint32_t addr) {
	uint32_t dmabuf = data;
	return sdio_io_rw_extended_block(func, 1, addr, 1, (uint8_t *)&dmabuf, 4);
}

int32_t sdio_io_read_multi_fifo_b(struct sdio_function *func, uint32_t addr, uint8_t *buf,
								  uint32_t len) {
	return sdio_io_rw_extended_block(func, 0, addr, 0, buf, len);
}

int32_t sdio_io_write_multi_fifo_b(struct sdio_function *func, uint32_t addr,
								   uint8_t *buf, uint32_t len) {
	return sdio_io_rw_extended_block(func, 1, addr, 0, buf, len);
}

int32_t sdio_io_read_multi_incr_b(struct sdio_function *func, uint32_t addr, uint8_t *buf,
								  uint32_t len) {
	return sdio_io_rw_extended_block(func, 0, addr, 1, buf, len);
}

int32_t sdio_io_write_multi_incr_b(struct sdio_function *func, uint32_t addr,
								   uint8_t *buf, uint32_t len) {
	return sdio_io_rw_extended_block(func, 1, addr, 1, buf, len);
}

static int32_t sdio_read_cccr(struct mmcsd_card *card) {
	int32_t ret;
	int32_t cccr_version;
	uint8_t data;

	memset(&card->cccr, 0, sizeof(struct sdio_cccr));
	data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_CCCR_REV, &ret);
	if (ret)
		goto out;

	cccr_version = data & 0x0f;
	if (cccr_version > SDIO_CCCR_REV_3_00) {
		pr_err("unrecognised CCCR structure version %d", cccr_version);
		return -EINVAL;
	}

	card->cccr.sdio_version = (data & 0xf0) >> 4;
	data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_CARD_CAPS, &ret);
	if (ret)
		goto out;

	if (data & SDIO_CCCR_CAP_SMB)
		card->cccr.multi_block = 1;
	if (data & SDIO_CCCR_CAP_LSC)
		card->cccr.low_speed = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		card->cccr.low_speed_4 = 1;
	if (data & SDIO_CCCR_CAP_4BLS)
		card->cccr.bus_width = 1;

	if (cccr_version >= SDIO_CCCR_REV_1_10) {
		data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_POWER_CTRL, &ret);
		if (ret)
			goto out;

		if (data & SDIO_POWER_SMPC)
			card->cccr.power_ctrl = 1;
	}

	if (cccr_version >= SDIO_CCCR_REV_1_20) {
		data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, &ret);
		if (ret)
			goto out;

		if (data & SDIO_SPEED_SHS)
			card->cccr.high_speed = 1;
	}

out:
	return ret;
}

static int32_t cistpl_funce_func0(struct mmcsd_card *card, const uint8_t *buf,
								  uint32_t size) {
	if (size < 0x04 || buf[0] != 0)
		return -EINVAL;

	/* TPLFE_FN0_BLK_SIZE */
	card->cis.func0_blk_size = buf[1] | (buf[2] << 8);

	/* TPLFE_MAX_TRAN_SPEED */
	card->cis.max_tran_speed = speed_value[(buf[3] >> 3) & 15] * speed_unit[buf[3] & 7];

	return 0;
}

static int32_t cistpl_funce_func(struct sdio_function *func, const uint8_t *buf,
								 uint32_t size) {
	uint32_t version;
	uint32_t min_size;

	version = func->card->cccr.sdio_version;
	min_size = (version == SDIO_SDIO_REV_1_00) ? 28 : 42;

	if (size < min_size || buf[0] != 1)
		return -EINVAL;

	/* TPLFE_MAX_BLK_SIZE */
	func->max_blk_size = buf[12] | (buf[13] << 8);

	/* TPLFE_ENABLE_TIMEOUT_VAL, present in ver 1.1 and above */
	if (version > SDIO_SDIO_REV_1_00)
		func->enable_timeout_val = (buf[28] | (buf[29] << 8)) * 10;
	else
		func->enable_timeout_val = 1000; /* 1000ms */

	return 0;
}

static int32_t sdio_read_cis(struct sdio_function *func) {
	int32_t ret;
	struct sdio_function_tuple *curr, **prev;
	uint32_t i, cisptr = 0;
	uint8_t data;
	uint8_t tpl_code, tpl_link;

	struct mmcsd_card *card = func->card;
	struct sdio_function *func0 = card->sdio_function[0];

	rte_assert(func0 != NULL);

	for (i = 0; i < 3; i++) {
		data = sdio_io_readb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_CIS + i,
							 &ret);
		if (ret)
			return ret;
		cisptr |= data << (i * 8);
	}

	prev = &func->tuples;

	do {
		tpl_code = sdio_io_readb(func0, cisptr++, &ret);
		if (ret)
			break;
		tpl_link = sdio_io_readb(func0, cisptr++, &ret);
		if (ret)
			break;

		if ((tpl_code == CISTPL_END) || (tpl_link == 0xff))
			break;

		if (tpl_code == CISTPL_NULL)
			continue;

		curr = malloc(sizeof(struct sdio_function_tuple) + tpl_link);
		if (!curr)
			return -ENOMEM;
		curr->data = (uint8_t *)curr + sizeof(struct sdio_function_tuple);

		for (i = 0; i < tpl_link; i++) {
			curr->data[i] = sdio_io_readb(func0, cisptr + i, &ret);
			if (ret)
				break;
		}
		if (ret) {
			kfree(curr);
			break;
		}

		switch (tpl_code) {
		case CISTPL_MANFID:
			if (tpl_link < 4) {
				pr_dbg("bad CISTPL_MANFID length");
			} else {
				if (func->num != 0) {
					func->manufacturer = curr->data[0];
					func->manufacturer |= curr->data[1] << 8;
					func->product = curr->data[2];
					func->product |= curr->data[3] << 8;
				} else {
					card->cis.manufacturer = curr->data[0];
					card->cis.manufacturer |= curr->data[1] << 8;
					card->cis.product = curr->data[2];
					card->cis.product |= curr->data[3] << 8;
				}
			}

			kfree(curr);
			break;
		case CISTPL_FUNCE:
			if (func->num != 0)
				ret = cistpl_funce_func(func, curr->data, tpl_link);
			else
				ret = cistpl_funce_func0(card, curr->data, tpl_link);

			if (ret) {
				pr_dbg("bad CISTPL_FUNCE size %u "
					   "type %u",
					   tpl_link, curr->data[0]);
			}

			break;
		case CISTPL_VERS_1:
			if (tpl_link < 2) {
				pr_dbg("CISTPL_VERS_1 too short");
			}
			break;
		default:
			/* this tuple is unknown to the core */
			curr->next = NULL;
			curr->code = tpl_code;
			curr->size = tpl_link;
			*prev = curr;
			prev = &curr->next;
			pr_dbg("function %d, CIS tuple code %#x, length %d", func->num, tpl_code,
				   tpl_link);
			break;
		}

		cisptr += tpl_link;
	} while (1);

	/*
	 * Link in all unknown tuples found in the common CIS so that
	 * drivers don't have to go digging in two places.
	 */
	if (func->num != 0)
		*prev = func0->tuples;

	return ret;
}

void sdio_free_cis(struct sdio_function *func) {
	struct sdio_function_tuple *tuple, *tmp;
	struct mmcsd_card *card = func->card;

	tuple = func->tuples;
	while (tuple && ((tuple != card->sdio_function[0]->tuples) || (!func->num))) {
		tmp = tuple;
		tuple = tuple->next;
		kfree(tmp);
	}

	func->tuples = NULL;
}

static int32_t sdio_read_fbr(struct sdio_function *func) {
	int32_t ret;
	uint8_t data;
	struct sdio_function *func0 = func->card->sdio_function[0];

	data = sdio_io_readb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_STD_FUNC_IF,
						 &ret);
	if (ret)
		goto err;

	data &= 0x0f;

	if (data == 0x0f) {
		data = sdio_io_readb(
			func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_STD_IF_EXT, &ret);
		if (ret)
			goto err;
	}

	func->func_code = data;

err:
	return ret;
}

static int32_t sdio_initialize_function(struct mmcsd_card *card, uint32_t func_num) {
	int32_t ret;
	struct sdio_function *func;

	rte_assert(func_num <= SDIO_MAX_FUNCTIONS);
	func = kmalloc(sizeof(struct sdio_function), GMF_KERNEL);
	if (!func) {
		pr_err("malloc sdio_function failed");
		ret = -ENOMEM;
		goto err;
	}
	memset(func, 0, sizeof(struct sdio_function));
	func->card = card;
	func->num = func_num;

	ret = sdio_read_fbr(func);
	if (ret)
		goto err1;

	ret = sdio_read_cis(func);
	if (ret)
		goto err1;

	/*
	 * product/manufacturer id is optional for function CIS, so
	 * copy it from the card structure as needed.
	 */
	if (func->product == 0) {
		func->manufacturer = card->cis.manufacturer;
		func->product = card->cis.product;
	}

	card->sdio_function[func_num] = func;
	return 0;

err1:
	sdio_free_cis(func);
	kfree(func);
	card->sdio_function[func_num] = NULL;
err:
	return ret;
}

static int32_t sdio_set_highspeed(struct mmcsd_card *card) {
	int32_t ret;
	uint8_t speed;

	if (!(card->host->flags & MMCSD_SUP_HIGHSPEED))
		return 0;

	if (!card->cccr.high_speed)
		return 0;

	speed = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, &ret);
	if (ret)
		return ret;

	speed |= SDIO_SPEED_EHS;
	ret = sdio_io_writeb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, speed);
	if (ret)
		return ret;

	card->flags |= CARD_FLAG_HIGHSPEED;
	return 0;
}

static int32_t sdio_set_bus_wide(struct mmcsd_card *card) {
	int32_t ret;
	uint8_t busif;

	if (!(card->host->flags & MMCSD_BUSWIDTH_4))
		return 0;

	if (card->cccr.low_speed && !card->cccr.bus_width)
		return 0;

	busif = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_BUS_IF, &ret);
	if (ret)
		return ret;

	busif |= SDIO_BUS_WIDTH_4BIT;

	ret = sdio_io_writeb(card->sdio_function[0], SDIO_REG_CCCR_BUS_IF, busif);
	if (ret)
		return ret;

	mmcsd_set_bus_width(card->host, MMCSD_BUS_WIDTH_4);

	return 0;
}

static int32_t sdio_register_card(struct mmcsd_card *card) {
	struct sdio_card *sc;
	struct sdio_private *sd;
	struct rte_list *l;

	sc = kmalloc(sizeof(struct sdio_card), GMF_KERNEL);
	if (sc == NULL) {
		pr_err("kmalloc sdio card failed");
		return -ENOMEM;
	}

	sc->card = card;
	rte_list_add_tail(&sc->list, &sdio_cards);
	if (rte_list_empty(&sdio_drivers)) {
		goto out;
	}

	rte_list_foreach(l, &sdio_drivers) {
		sd = (struct sdio_private *)rte_list_entry(l, struct sdio_private, list);
		if (sdio_match_card(card, sd->drv->id)) {
			sd->drv->probe(card);
		}
	}

out:
	return 0;
}

static int32_t sdio_init_card(struct mmcsd_host *host, uint32_t ocr) {
	int32_t err = 0;
	int32_t i, function_num;
	uint32_t cmd5_resp;
	struct mmcsd_card *card;

	err = sdio_io_send_op_cond(host, ocr, &cmd5_resp);
	if (err)
		goto err;

	if (controller_is_spi(host)) {
		err = mmcsd_spi_use_crc(host, host->spi_use_crc);
		if (err)
			goto err;
	}

	function_num = (cmd5_resp & 0x70000000) >> 28;

	card = kmalloc(sizeof(struct mmcsd_card), GMF_KERNEL);
	if (!card) {
		pr_err("malloc card failed");
		err = -ENOMEM;
		goto err;
	}
	memset(card, 0, sizeof(struct mmcsd_card));
	card->card_type = CARD_TYPE_SDIO;
	card->sdio_function_num = function_num;
	card->host = host;
	host->card = card;

	card->sdio_function[0] = malloc(sizeof(struct sdio_function));
	if (!card->sdio_function[0]) {
		pr_err("malloc sdio_func0 failed");
		err = -ENOMEM;
		goto err1;
	}
	memset(card->sdio_function[0], 0, sizeof(struct sdio_function));
	card->sdio_function[0]->card = card;
	card->sdio_function[0]->num = 0;

	if (!controller_is_spi(host)) {
		err = mmcsd_get_card_addr(host, &card->rca);
		if (err)
			goto err2;

		mmcsd_set_bus_mode(host, MMCSD_BUSMODE_PUSHPULL);
	}

	if (!controller_is_spi(host)) {
		err = mmcsd_select_card(card);
		if (err)
			goto err2;
	}

	err = sdio_read_cccr(card);
	if (err)
		goto err2;

	err = sdio_read_cis(card->sdio_function[0]);
	if (err)
		goto err2;

	err = sdio_set_highspeed(card);
	if (err)
		goto err2;

	if (card->flags & CARD_FLAG_HIGHSPEED) {
		mmcsd_set_clock(host, card->host->freq_max > 50000000 ? 50000000
															  : card->host->freq_max);
	} else {
		mmcsd_set_clock(host, card->cis.max_tran_speed);
	}

	err = sdio_set_bus_wide(card);
	if (err)
		goto err2;

	for (i = 1; i < function_num + 1; i++) {
		err = sdio_initialize_function(card, i);
		if (err)
			goto err3;
	}

	/* register sdio card */
	err = sdio_register_card(card);
	if (err) {
		goto err3;
	}

	return 0;

err3:
	if (host->card) {
		for (i = 1; i < host->card->sdio_function_num + 1; i++) {
			if (host->card->sdio_function[i]) {
				sdio_free_cis(host->card->sdio_function[i]);
				kfree(host->card->sdio_function[i]);
				host->card->sdio_function[i] = NULL;
			}
		}
	}
err2:
	if (host->card && host->card->sdio_function[0]) {
		sdio_free_cis(host->card->sdio_function[0]);
		kfree(host->card->sdio_function[0]);
		host->card->sdio_function[0] = NULL;
	}
err1:
	if (host->card) {
		kfree(host->card);
		host->card = NULL;
	}
err:
	pr_err("error %d while initialising SDIO card", err);

	return err;
}

int32_t init_sdio(struct mmcsd_host *host, uint32_t ocr) {
	int32_t err;
	uint32_t current_ocr;

	rte_assert(host != NULL);
	if (ocr & 0x7F) {
		pr_warn("Card ocr below the defined voltage rang.");
		ocr &= ~0x7F;
	}

	if (ocr & VDD_165_195) {
		pr_warn("Can't support the low voltage SDIO card.");
		ocr &= ~VDD_165_195;
	}

	current_ocr = mmcsd_select_voltage(host, ocr);
	if (!current_ocr) {
		err = -EINVAL;
		goto err;
	}

	err = sdio_init_card(host, current_ocr);
	if (err)
		goto err;

	return 0;

err:
	pr_err("init SDIO card failed");
	return err;
}

static void sdio_irq_thread(void *param) {
	int32_t i, ret;
	uint8_t pending;
	struct mmcsd_card *card;
	struct mmcsd_host *host = (struct mmcsd_host *)param;
	rte_assert(host != NULL);
	card = host->card;
	rte_assert(card != NULL);

	for (;;) {
		if (!tx_semaphore_get(&host->sdio_irq_sem, TX_WAIT_FOREVER)) {
			mmcsd_host_lock(host);
			pending =
				sdio_io_readb(host->card->sdio_function[0], SDIO_REG_CCCR_INT_PEND, &ret);
			if (ret) {
				mmcsd_dbg("error %d reading SDIO_REG_CCCR_INT_PEND\n", ret);
				goto out;
			}

			for (i = 1; i <= 7; i++) {
				if (pending & (1 << i)) {
					struct sdio_function *func = card->sdio_function[i];
					if (!func) {
						mmcsd_dbg("pending IRQ for non-existant function\n");
						goto out;
					} else if (func->irq_handler) {
						func->irq_handler(func);
					} else {
						mmcsd_dbg("pending IRQ with no register handler\n");
						goto out;
					}
				}
			}

		out:
			mmcsd_host_unlock(host);
			if (host->flags & MMCSD_SUP_SDIO_IRQ)
				MMCSD_ENABLE_SDIO_IRQ(host, 1);
			continue;
		}
	}
}

static int32_t sdio_irq_thread_create(struct mmcsd_card *card) {
	struct mmcsd_host *host = card->host;

	/* init semaphore and create sdio irq processing thread */
	if (!host->sdio_irq_num) {
		host->sdio_irq_num++;
		tx_semaphore_create(&host->sdio_irq_sem, (CHAR *)"sdio_irq", 0);
		tx_thread_spawn(&host->sdio_irq_thread, "sdio_irq", sdio_irq_thread, host,
						sdio_thread_stack, CONFIG_SDIO_STACK_SIZE,
						CONFIG_SDIO_THREAD_PRIORITY, CONFIG_SDIO_THREAD_PRIORITY,
						TX_NO_TIME_SLICE, TX_AUTO_START);
	}

	return 0;
}

static int32_t sdio_irq_thread_delete(struct mmcsd_card *card) {
	struct mmcsd_host *host = card->host;

	rte_assert(host->sdio_irq_num > 0);
	host->sdio_irq_num--;
	if (!host->sdio_irq_num) {
		if (host->flags & MMCSD_SUP_SDIO_IRQ)
			MMCSD_ENABLE_SDIO_IRQ(host, 0);
		tx_semaphore_delete(&host->sdio_irq_sem);
		tx_thread_terminate(&host->sdio_irq_thread);
	}

	return 0;
}

int32_t sdio_attach_irq(struct sdio_function *func, sdio_irq_handler_t *handler) {
	int32_t ret;
	uint8_t reg;
	struct sdio_function *func0;

	rte_assert(func != NULL);
	rte_assert(func->card != NULL);
	func0 = func->card->sdio_function[0];

	mmcsd_dbg("SDIO: enabling IRQ for function %d\n", func->num);
	if (func->irq_handler) {
		mmcsd_dbg("SDIO: IRQ for already in use.\n");
		return -EBUSY;
	}

	reg = sdio_io_readb(func0, SDIO_REG_CCCR_INT_EN, &ret);
	if (ret)
		return ret;

	reg |= 1 << func->num;
	reg |= 1; /* Master interrupt enable */

	ret = sdio_io_writeb(func0, SDIO_REG_CCCR_INT_EN, reg);
	if (ret)
		return ret;

	func->irq_handler = handler;
	ret = sdio_irq_thread_create(func->card);
	if (ret)
		func->irq_handler = NULL;

	return ret;
}

int32_t sdio_detach_irq(struct sdio_function *func) {
	int32_t ret;
	uint8_t reg;
	struct sdio_function *func0;

	rte_assert(func != NULL);
	rte_assert(func->card != NULL);
	func0 = func->card->sdio_function[0];
	mmcsd_dbg("SDIO: disabling IRQ for function %d\n", func->num);

	if (func->irq_handler) {
		func->irq_handler = NULL;
		sdio_irq_thread_delete(func->card);
	}

	reg = sdio_io_readb(func0, SDIO_REG_CCCR_INT_EN, &ret);
	if (ret)
		return ret;

	reg &= ~(1 << func->num);

	/* Disable master interrupt with the last function interrupt */
	if (!(reg & 0xFE))
		reg = 0;

	ret = sdio_io_writeb(func0, SDIO_REG_CCCR_INT_EN, reg);
	if (ret)
		return ret;

	return 0;
}

void sdio_irq_wakeup(struct mmcsd_host *host) {
	if (host->flags & MMCSD_SUP_SDIO_IRQ)
		MMCSD_ENABLE_SDIO_IRQ(host, 0);
	tx_semaphore_ceiling_put(&host->sdio_irq_sem, 1);
}

int32_t sdio_enable_func(struct sdio_function *func) {
	int32_t ret;
	uint8_t reg;
	uint32_t timeout;
	struct sdio_function *func0;

	rte_assert(func != NULL);
	rte_assert(func->card != NULL);

	func0 = func->card->sdio_function[0];
	mmcsd_dbg("SDIO: enabling function %d\n", func->num);

	reg = sdio_io_readb(func0, SDIO_REG_CCCR_IO_EN, &ret);
	if (ret)
		goto err;

	reg |= 1 << func->num;
	ret = sdio_io_writeb(func0, SDIO_REG_CCCR_IO_EN, reg);
	if (ret)
		goto err;

	timeout = _tx_timer_system_clock +
			  func->enable_timeout_val * TX_TIMER_TICKS_PER_SECOND / 1000;
	while (1) {
		reg = sdio_io_readb(func0, SDIO_REG_CCCR_IO_RDY, &ret);
		if (ret)
			goto err;
		if (reg & (1 << func->num))
			break;
		ret = -ETIMEDOUT;
		if (_tx_timer_system_clock > timeout)
			goto err;
	}

	mmcsd_dbg("SDIO: enabled function successfull\n");
	return 0;

err:
	mmcsd_dbg("SDIO: failed to enable function %d\n", func->num);
	return ret;
}

int32_t sdio_disable_func(struct sdio_function *func) {
	int32_t ret;
	uint8_t reg;
	struct sdio_function *func0;

	rte_assert(func != NULL);
	rte_assert(func->card != NULL);

	func0 = func->card->sdio_function[0];
	mmcsd_dbg("SDIO: disabling function %d\n", func->num);

	reg = sdio_io_readb(func0, SDIO_REG_CCCR_IO_EN, &ret);
	if (ret)
		goto err;

	reg &= ~(1 << func->num);
	ret = sdio_io_writeb(func0, SDIO_REG_CCCR_IO_EN, reg);
	if (ret)
		goto err;

	mmcsd_dbg("SDIO: disabled function successfull\n");
	return 0;

err:
	mmcsd_dbg("SDIO: failed to disable function %d\n", func->num);
	return -EIO;
}

void sdio_set_drvdata(struct sdio_function *func, void *data) {
	func->priv = data;
}

void *sdio_get_drvdata(struct sdio_function *func) {
	return func->priv;
}

int32_t sdio_set_block_size(struct sdio_function *func, uint32_t blksize) {
	int32_t ret;
	struct sdio_function *func0 = func->card->sdio_function[0];

	if (blksize > func->card->host->max_blk_size)
		return -EINVAL;

	if (blksize == 0) {
		blksize = MIN(func->max_blk_size, func->card->host->max_blk_size);
		blksize = MIN(blksize, (uint32_t)512u);
	}

	ret = sdio_io_writeb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_BLKSIZE,
						 blksize & 0xff);
	if (ret)
		return ret;
	ret = sdio_io_writeb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_BLKSIZE + 1,
						 (blksize >> 8) & 0xff);
	if (ret)
		return ret;
	func->cur_blk_size = blksize;

	return 0;
}

static inline int32_t sdio_match_card(struct mmcsd_card *card,
									  const struct sdio_device_id *id) {
	uint8_t num = 1;

	if ((id->manufacturer != SDIO_ANY_MAN_ID) &&
		(id->manufacturer != card->cis.manufacturer))
		return 0;

	while (num <= card->sdio_function_num) {
		if ((id->product != SDIO_ANY_PROD_ID) &&
			(id->product == card->sdio_function[num]->product))
			return 1;
		num++;
	}

	return 0;
}

static struct mmcsd_card *sdio_match_driver(struct sdio_device_id *id) {
	struct rte_list *l;
	struct sdio_card *sc;
	struct mmcsd_card *card;

	rte_list_foreach(l, &sdio_cards) {
		sc = (struct sdio_card *)rte_list_entry(l, struct sdio_card, list);
		card = sc->card;

		if (sdio_match_card(card, id)) {
			return card;
		}
	}

	return NULL;
}

int32_t sdio_register_driver(struct sdio_driver *driver) {
	struct sdio_private *sd;
	struct mmcsd_card *card;

	sd = kmalloc(sizeof(struct sdio_private), GMF_KERNEL);
	if (sd == NULL) {
		pr_err("malloc sdio driver failed");
		return -ENOMEM;
	}

	sd->drv = driver;
	rte_list_add_tail(&sd->list, &sdio_drivers);
	if (!rte_list_empty(&sdio_cards)) {
		card = sdio_match_driver(driver->id);
		if (card != NULL) {
			return driver->probe(card);
		}
	}

	return -ENODATA;
}

int32_t sdio_unregister_driver(struct sdio_driver *driver) {
	struct rte_list *l;
	struct sdio_private *sd = NULL;
	struct mmcsd_card *card;

	rte_list_foreach(l, &sdio_drivers) {
		sd = (struct sdio_private *)rte_list_entry(l, struct sdio_private, list);
		if (sd->drv != driver) {
			sd = NULL;
		}
	}

	if (sd == NULL) {
		pr_err("SDIO driver %s not register", driver->name);
		return -EINVAL;
	}

	if (!rte_list_empty(&sdio_cards)) {
		card = sdio_match_driver(driver->id);
		if (card != NULL) {
			driver->remove(card);
			rte_list_del(&sd->list);
			kfree(sd);
		}
	}

	return 0;
}

void sdio_init(void) {}
