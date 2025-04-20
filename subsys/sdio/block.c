/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2011-07-25     weety         first version
 * 2023-08-08     GuEe-GUI      port to the block
 */

#define pr_fmt(fmt) "[SDIO]: " fmt "\n"

#include <errno.h>
#include <tx_api.h>
#include <tx_timer.h>
#include <service/malloc.h>
#include <base/lib/string.h>
#include <base/log.h>
#include <drivers/blkdev.h>
#include <drivers/blkdev.h>
#include <drivers/sdio/mmcsd_core.h>


#ifndef CONFIG_SECTOR_SIZE
#define CONFIG_SECTOR_SIZE  512
#endif

struct mmcsd_blk_device {
    struct block_device blkdev;
	struct mmcsd_card *card;

	size_t max_req_size;
	// struct device_blk_geometry geometry;
};

#define raw_to_mmcsd_blk(raw) rte_container_of(raw, struct mmcsd_blk_device, blkdev)

static int __send_status(struct mmcsd_card *card, uint32_t *status, unsigned retries) {
	int err;
	struct mmcsd_cmd cmd;

	cmd.busy_timeout = 0;
	cmd.cmd_code = SEND_STATUS;
	cmd.arg = card->rca << 16;
	cmd.flags = RESP_R1 | CMD_AC;
	err = mmcsd_send_cmd(card->host, &cmd, retries);
	if (err)
		return err;

	if (status)
		*status = cmd.resp[0];

	return 0;
}

static int card_busy_detect(struct mmcsd_card *card, unsigned int timeout_ms,
							uint32_t *resp_errs) {
	int timeout = TX_MSEC(timeout_ms);
	int err = 0;
	uint32_t status;
	uint32_t start;

	start = _tx_timer_system_clock;
	do {
		bool out = (int)(_tx_timer_system_clock - start) > timeout;

		err = __send_status(card, &status, 5);
		if (err) {
			pr_err("error %d requesting status", err);
			return err;
		}

		/* Accumulate any response error bits seen */
		if (resp_errs)
			*resp_errs |= status;

		if (out) {
			pr_err("wait card busy timeout");
			return -ETIMEDOUT;
		}
		/*
		 * Some cards mishandle the status bits,
		 * so make sure to check both the busy
		 * indication and the card state.
		 */
	} while (!(status & R1_READY_FOR_DATA) || (R1_CURRENT_STATE(status) == 7));

	return err;
}

int mmcsd_num_wr_blocks(struct mmcsd_card *card) {
	struct mmcsd_req req;
	struct mmcsd_cmd cmd;
	struct mmcsd_data data;
	uint32_t timeout_us;
	uint32_t blocks;
    int err;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = APP_CMD;
	cmd.arg = card->rca << 16;
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;

	err = mmcsd_send_cmd(card->host, &cmd, 0);
	if (err)
		return -EINVAL;
	if (!controller_is_spi(card->host) && !(cmd.resp[0] & R1_APP_CMD))
		return -EINVAL;

	memset(&cmd, 0, sizeof(struct mmcsd_cmd));
	cmd.cmd_code = SD_APP_SEND_NUM_WR_BLKS;
	cmd.arg = 0;
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;

	memset(&data, 0, sizeof(struct mmcsd_data));
	data.timeout_ns = card->tacc_ns * 100;
	data.timeout_clks = card->tacc_clks * 100;

	timeout_us = data.timeout_ns / 1000;
	timeout_us += data.timeout_clks * 1000 / (card->host->io_cfg.clock / 1000);
	if (timeout_us > 100000) {
		data.timeout_ns = 100000000;
		data.timeout_clks = 0;
	}

	data.blksize = 4;
	data.blks = 1;
	data.flags = DATA_DIR_READ;
	data.buf = &blocks;

	memset(&req, 0, sizeof(struct mmcsd_req));
	req.cmd = &cmd;
	req.data = &data;

	mmcsd_send_request(card->host, &req);
	if (cmd.err || data.err)
		return -EINVAL;

	return blocks;
}

static int mmcsd_req_blk(struct mmcsd_card *card, uint32_t sector, void *buf, size_t blks,
						 uint8_t dir) {
	struct mmcsd_cmd cmd = {0}, stop = {0};
	struct mmcsd_data data = {0};
	struct mmcsd_req req = {0};
	struct mmcsd_host *host = card->host;
	uint32_t r_cmd, w_cmd;

	req.cmd = &cmd;
	req.data = &data;
	cmd.arg = sector;
	if (!(card->flags & CARD_FLAG_SDHC)) {
		cmd.arg <<= 9;
	}
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_ADTC;
	data.blksize = CONFIG_SECTOR_SIZE;
	data.blks = blks;

	if (blks > 1) {
		if (!controller_is_spi(card->host) || !dir) {
			req.stop = &stop;
			stop.cmd_code = STOP_TRANSMISSION;
			stop.arg = 0;
			stop.flags = RESP_SPI_R1B | RESP_R1B | CMD_AC;
		}
		r_cmd = READ_MULTIPLE_BLOCK;
		w_cmd = WRITE_MULTIPLE_BLOCK;
	} else {
		req.stop = NULL;
		r_cmd = READ_SINGLE_BLOCK;
		w_cmd = WRITE_BLOCK;
	}

	mmcsd_host_lock(host);
	if (!controller_is_spi(card->host) && (card->flags & 0x8000)) {
		/* last request is WRITE,need check busy */
		card_busy_detect(card, 10000, NULL);
	}

	if (!dir) {
		cmd.cmd_code = r_cmd;
		data.flags |= DATA_DIR_READ;
		card->flags &= 0x7fff;
	} else {
		cmd.cmd_code = w_cmd;
		data.flags |= DATA_DIR_WRITE;
		card->flags |= 0x8000;
	}

	mmcsd_set_data_timeout(&data, card);
	data.buf = buf;

	mmcsd_send_request(host, &req);
	mmcsd_host_unlock(host);
	if (cmd.err || data.err || stop.err) {
		pr_err("mmcsd request blocks error %"PRId32",%"PRId32",%"PRId32", 0x%08"PRIx32",0x%08"PRIx32"\n", 
            cmd.err, data.err, stop.err, data.flags, sector);
		return -EINVAL;
	}

	return 0;
}

int mmcsd_set_blksize(struct mmcsd_card *card) {
	struct mmcsd_cmd cmd;
	int err;

	/* Block-addressed cards ignore MMC_SET_BLOCKLEN. */
	if (card->flags & CARD_FLAG_SDHC)
		return 0;

	mmcsd_host_lock(card->host);
	cmd.cmd_code = SET_BLOCKLEN;
	cmd.arg = 512;
	cmd.flags = RESP_SPI_R1 | RESP_R1 | CMD_AC;
	err = mmcsd_send_cmd(card->host, &cmd, 5);
	mmcsd_host_unlock(card->host);
	if (err) {
		pr_err("MMCSD: unable to set block size to %"PRIu32": %d", cmd.arg, err);
		return -EINVAL;
	}

	return 0;
}

static int mmcsd_blkdev_request(struct device *dev, struct blkdev_req *req) {
    struct mmcsd_card *card = dev_get_private(dev);

	return mmcsd_req_blk(card, req->blkno, req->buffer, 
        req->blkcnt, req->op);
}

static int mmcsd_blkdev_control(struct device *dev, unsigned int cmd, void *buf) {
    struct mmcsd_card *card = dev_get_private(dev);
	int ret = 0;

	switch (cmd) {
	case BLKDEV_IOC_GET_BLKCOUNT:
		(*(uint32_t *)buf) = card->card_sec_cnt;
		break;
	case BLKDEV_IOC_GET_BLKSIZE:
	case BLKDEV_IOC_GET_ERASE_BLKSIZE:
		(*(uint32_t *)buf) = card->card_blksize;
		break;
	case BLKDEV_IOC_SYNC:
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

int mmcsd_blkdev_probe(struct mmcsd_card *card) {
    static uint8_t devno;
    struct block_device *bdev;
    char name[] = {"mmcblk0"};
    char *p;
    int err;

    if (card->card_type > CARD_TYPE_SD)
        return -EINVAL;

    if (card->blk_dev != NULL)
        return -EEXIST;
    
    bdev = kzalloc(sizeof(*bdev) + sizeof(name) + 1, 0);
    if (bdev == NULL)
        return -ENOMEM;
    
    name[6] += devno;
    p = (char *)(bdev + 1);
    strcpy(p, name);

    bdev->private_data = card;
    bdev->name = p;
    bdev->request = mmcsd_blkdev_request;
    bdev->control = mmcsd_blkdev_control;
    err = device_register((struct device *)bdev);
    if (!err) {
        pr_info("%s register device(%p) success\n", name, bdev);
        card->blk_dev = bdev;
        devno++;
        return 0;
    }
    
    kfree(bdev);
    return err;
}

int mmcsd_blkdev_remove(struct mmcsd_card *card) {
    if (card->blk_dev == NULL)
        return 0;

    device_unregister(card->blk_dev);
    kfree(card->blk_dev);
    return 0;
}
