/*
 * Copyright 2022,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define pr_fmt(fmt) "[sd_ops]: " fmt

#include <errno.h>
#include "tx_api.h"

#include "basework/byteorder.h"
#include "basework/log.h"

#include "subsys/sd/sd.h"
#include "subsys/sd/sd_spec.h"
#include "subsys/sd/sdhc.h"
#include "subsys/sd/private/sd_utils.h"

#include "drivers/blkdev.h"

/* Read card status. Return 0 if card is inactive */
int sdmmc_read_status(struct sd_card *card) {
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = SD_SEND_STATUS;
	cmd.arg = 0;
	if (!card->host_props.is_spi) {
		cmd.arg = (card->relative_addr << 16U);
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R2);
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		return SD_RETRY;
	}
	if (card->host_props.is_spi) {
		/* Check R2 response bits */
		if ((cmd.response[0U] & SDHC_SPI_R2_CARD_LOCKED) ||
			(cmd.response[0U] & SDHC_SPI_R2_UNLOCK_FAIL)) {
			return -EACCES;
		} else if ((cmd.response[0U] & SDHC_SPI_R2_WP_VIOLATION) ||
				   (cmd.response[0U] & SDHC_SPI_R2_ERASE_PARAM) ||
				   (cmd.response[0U] & SDHC_SPI_R2_OUT_OF_RANGE)) {
			return -EINVAL;
		} else if ((cmd.response[0U] & SDHC_SPI_R2_ERR) ||
				   (cmd.response[0U] & SDHC_SPI_R2_CC_ERR) ||
				   (cmd.response[0U] & SDHC_SPI_R2_ECC_FAIL)) {
			return -EIO;
		}
		/* Otherwise, no error in R2 response */
		return 0;
	}
	/* Otherwise, check native card response */
	if ((cmd.response[0U] & SD_R1_RDY_DATA) &&
		(SD_R1_CURRENT_STATE(cmd.response[0U]) == SDMMC_R1_TRANSFER)) {
		return 0;
	}
	/* Valid response, the card is busy */
	return -EBUSY;
}

/* Waits for SD card to be ready for data. Returns 0 if card is ready */
int sdmmc_wait_ready(struct sd_card *card) {
	int ret, timeout = CONFIG_SD_DATA_TIMEOUT * 1000;

	do {
		if (!sdhc_card_busy(card->sdhc)) {
			/* Check card status */
			ret = sd_retry(sdmmc_read_status, card, CONFIG_SD_RETRY_COUNT);
			if (ret == 0) {
				return 0;
			}
			if (ret == -ETIMEDOUT) {
				/* If this check timed out, then the total
				 * time elapsed in microseconds is
				 * SD_CMD_TIMEOUT * SD_RETRY_COUNT * 1000
				 */
				timeout -= (CONFIG_SD_CMD_TIMEOUT * CONFIG_SD_RETRY_COUNT) * 1000;
			}
		}
		/* Delay 125us before polling again */
		tx_os_nanosleep(125000);
		timeout -= 125;
	} while (timeout > 0);
	return -EBUSY;
}

static inline void sdmmc_decode_csd(struct sd_csd *csd, uint32_t *raw_csd,
									uint32_t *blk_count, uint16_t *blk_size) {
	uint32_t tmp_blk_count;
	uint16_t tmp_blk_size;

	csd->csd_structure = (uint8_t)((raw_csd[3U] & 0xC0000000U) >> 30U);
	csd->read_time1 = (uint8_t)((raw_csd[3U] & 0xFF0000U) >> 16U);
	csd->read_time2 = (uint8_t)((raw_csd[3U] & 0xFF00U) >> 8U);
	csd->xfer_rate = (uint8_t)(raw_csd[3U] & 0xFFU);
	csd->cmd_class = (uint16_t)((raw_csd[2U] & 0xFFF00000U) >> 20U);
	csd->read_blk_len = (uint8_t)((raw_csd[2U] & 0xF0000U) >> 16U);
	if (raw_csd[2U] & 0x8000U) {
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	}
	if (raw_csd[2U] & 0x4000U) {
		csd->flags |= SD_CSD_READ_BLK_PARTIAL_FLAG;
	}
	if (raw_csd[2U] & 0x2000U) {
		csd->flags |= SD_CSD_READ_BLK_MISALIGN_FLAG;
	}
	if (raw_csd[2U] & 0x1000U) {
		csd->flags |= SD_CSD_DSR_IMPLEMENTED_FLAG;
	}

	switch (csd->csd_structure) {
	case 0:
		csd->device_size = (uint32_t)((raw_csd[2U] & 0x3FFU) << 2U);
		csd->device_size |= (uint32_t)((raw_csd[1U] & 0xC0000000U) >> 30U);
		csd->read_current_min = (uint8_t)((raw_csd[1U] & 0x38000000U) >> 27U);
		csd->read_current_max = (uint8_t)((raw_csd[1U] & 0x7000000U) >> 24U);
		csd->write_current_min = (uint8_t)((raw_csd[1U] & 0xE00000U) >> 20U);
		csd->write_current_max = (uint8_t)((raw_csd[1U] & 0x1C0000U) >> 18U);
		csd->dev_size_mul = (uint8_t)((raw_csd[1U] & 0x38000U) >> 15U);

		/* Get card total block count and block size. */
		tmp_blk_count = ((csd->device_size + 1U) << (csd->dev_size_mul + 2U));
		tmp_blk_size = (1U << (csd->read_blk_len));
		if (tmp_blk_size != SDMMC_DEFAULT_BLOCK_SIZE) {
			tmp_blk_count = (tmp_blk_count * tmp_blk_size);
			tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;
			tmp_blk_count = (tmp_blk_count / tmp_blk_size);
		}
		if (blk_count) {
			*blk_count = tmp_blk_count;
		}
		if (blk_size) {
			*blk_size = tmp_blk_size;
		}
		break;
	case 1:
		tmp_blk_size = SDMMC_DEFAULT_BLOCK_SIZE;

		csd->device_size = (uint32_t)((raw_csd[2U] & 0x3FU) << 16U);
		csd->device_size |= (uint32_t)((raw_csd[1U] & 0xFFFF0000U) >> 16U);

		tmp_blk_count = ((csd->device_size + 1U) * 1024U);
		if (blk_count) {
			*blk_count = tmp_blk_count;
		}
		if (blk_size) {
			*blk_size = tmp_blk_size;
		}
		break;
	default:
		break;
	}

	if ((uint8_t)((raw_csd[1U] & 0x4000U) >> 14U)) {
		csd->flags |= SD_CSD_ERASE_BLK_EN_FLAG;
	}
	csd->erase_size = (uint8_t)((raw_csd[1U] & 0x3F80U) >> 7U);
	csd->write_prtect_size = (uint8_t)(raw_csd[1U] & 0x7FU);
	csd->write_speed_factor = (uint8_t)((raw_csd[0U] & 0x1C000000U) >> 26U);
	csd->write_blk_len = (uint8_t)((raw_csd[0U] & 0x3C00000U) >> 22U);
	if ((uint8_t)((raw_csd[0U] & 0x200000U) >> 21U)) {
		csd->flags |= SD_CSD_WRITE_BLK_PARTIAL_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x8000U) >> 15U)) {
		csd->flags |= SD_CSD_FILE_FMT_GRP_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x4000U) >> 14U)) {
		csd->flags |= SD_CSD_COPY_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x2000U) >> 13U)) {
		csd->flags |= SD_CSD_PERMANENT_WRITE_PROTECT_FLAG;
	}
	if ((uint8_t)((raw_csd[0U] & 0x1000U) >> 12U)) {
		csd->flags |= SD_CSD_TMP_WRITE_PROTECT_FLAG;
	}
	csd->file_fmt = (uint8_t)((raw_csd[0U] & 0xC00U) >> 10U);
}

static inline void sdmmc_decode_cid(struct sd_cid *cid, uint32_t *raw_cid) {
	cid->manufacturer = (uint8_t)((raw_cid[3U] & 0xFF000000U) >> 24U);
	cid->application = (uint16_t)((raw_cid[3U] & 0xFFFF00U) >> 8U);

	cid->name[0U] = (uint8_t)((raw_cid[3U] & 0xFFU));
	cid->name[1U] = (uint8_t)((raw_cid[2U] & 0xFF000000U) >> 24U);
	cid->name[2U] = (uint8_t)((raw_cid[2U] & 0xFF0000U) >> 16U);
	cid->name[3U] = (uint8_t)((raw_cid[2U] & 0xFF00U) >> 8U);
	cid->name[4U] = (uint8_t)((raw_cid[2U] & 0xFFU));

	cid->version = (uint8_t)((raw_cid[1U] & 0xFF000000U) >> 24U);

	cid->ser_num = (uint32_t)((raw_cid[1U] & 0xFFFFFFU) << 8U);
	cid->ser_num |= (uint32_t)((raw_cid[0U] & 0xFF000000U) >> 24U);

	cid->date = (uint16_t)((raw_cid[0U] & 0xFFF00U) >> 8U);
}

/* Reads card id/csd register (in SPI mode) */
static int sdmmc_spi_read_cxd(struct sd_card *card, uint32_t opcode, uint32_t *cxd) {
	struct sdhc_command cmd;
	struct sdhc_data data;
	int ret, i;
	/* Use internal card buffer for data transfer */
	uint32_t *cxd_be = (uint32_t *)card->card_buffer;

	cmd.opcode = opcode;
	cmd.arg = 0;
	cmd.response_type = SD_SPI_RSP_TYPE_R1;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	/* CID/CSD is 16 bytes */
	data.block_addr = 0; /* Unused set to 0 */
	data.block_size = 16;
	data.blocks = 1U;
	data.data = cxd_be;
	data.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		pr_err("CMD%d failed: %d\n", opcode, ret);
	}
	/* Swap endianness of CXD */
	for (i = 0; i < 4; i++) {
		cxd[3 - i] = rte_be32_to_cpu(cxd_be[i]);
	}
	return 0;
}

/* Reads card id/csd register (native SD mode */
static int sdmmc_read_cxd(struct sd_card *card, uint32_t opcode, uint32_t rca,
						  uint32_t *cxd) {
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = opcode;
	cmd.arg = (rca << 16);
	cmd.response_type = SD_RSP_TYPE_R2;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		pr_err("CMD%d failed: %d\n", opcode, ret);
		return ret;
	}
	/* CSD/CID is 16 bytes */
	memcpy(cxd, cmd.response, 16);
	return 0;
}

/* Read card specific data register */
int sdmmc_read_csd(struct sd_card *card) {
	int ret;
	uint32_t csd[4];
	/* Keep CSD on stack for reduced RAM usage */
	struct sd_csd card_csd = {0};

	if (card->host_props.is_spi && RTE_IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		ret = sdmmc_spi_read_cxd(card, SD_SEND_CSD, csd);
	} else if (RTE_IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		ret = sdmmc_read_cxd(card, SD_SEND_CSD, card->relative_addr, csd);
	} else {
		/* The host controller must run in either native or SPI mode */
		return -ENOTSUP;
	}
	if (ret) {
		return ret;
	}
	sdmmc_decode_csd(&card_csd, csd, &card->block_count, &card->block_size);
	pr_dbg("Card block count %d, block size %d\n", card->block_count, card->block_size);
	return 0;
}

/* Reads card identification register, and decodes it */
int card_read_cid(struct sd_card *card) {
	uint32_t cid[4];
	int ret;
#if defined(CONFIG_SDMMC_STACK) || defined(CONFIG_SDIO_STACK)
	/* Keep CID on stack for reduced RAM usage */
	struct sd_cid card_cid = {0};
#endif

	if (card->host_props.is_spi && RTE_IS_ENABLED(CONFIG_SDHC_SUPPORTS_SPI_MODE)) {
		ret = sdmmc_spi_read_cxd(card, SD_SEND_CID, cid);
	} else if (RTE_IS_ENABLED(CONFIG_SDHC_SUPPORTS_NATIVE_MODE)) {
		ret = sdmmc_read_cxd(card, SD_ALL_SEND_CID, 0, cid);
	} else {
		/* The host controller must run in either native or SPI mode */
		return -ENOTSUP;
	}
	if (ret) {
		return ret;
	}

#if defined(CONFIG_MMC_STACK)
	if (card->type == CARD_MMC) {
		pr_info("CID decoding not supported for MMC\n");
		return 0;
	}
#endif
#if defined(CONFIG_SDMMC_STACK) || defined(CONFIG_SDIO_STACK)
	/* Decode SD CID */
	sdmmc_decode_cid(&card_cid, cid);
	pr_dbg("Card MID: 0x%x, OID: %c%c\n", card_cid.manufacturer,
		   ((char *)&card_cid.application)[0], ((char *)&card_cid.application)[1]);
#endif

	return 0;
}

/*
 * Implements signal voltage switch procedure described in section 3.6.1 of
 * SD host controller specification.
 */
int sdmmc_switch_voltage(struct sd_card *card) {
	int ret, sd_clock;
	struct sdhc_command cmd;

	/* Check to make sure card supports 1.8V */
	if (!(card->flags & SD_1800MV_FLAG)) {
		/* Do not attempt to switch voltages */
		pr_warn("SD card reports as SDHC/SDXC, but does not support 1.8V\n");
		return 0;
	}
	/* Send CMD11 to request a voltage switch */
	cmd.opcode = SD_VOL_SWITCH;
	cmd.arg = 0U;
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		pr_err("CMD11 failed\n");
		return ret;
	}
	/* Check R1 response for error */
	ret = sd_check_response(&cmd);
	if (ret) {
		pr_err("SD response to CMD11 indicates error\n");
		return ret;
	}
	/*
	 * Card should drive CMD and DAT[3:0] signals low at the next clock
	 * cycle. Some cards will only drive these
	 * lines low briefly, so we should check as soon as possible
	 */
	if (!(sdhc_card_busy(card->sdhc))) {
		/* Delay 1ms to allow card to drive lines low */
		sd_delay(1);
		if (!sdhc_card_busy(card->sdhc)) {
			/* Card did not drive CMD and DAT lines low */
			pr_err("Card did not drive DAT lines low\n");
			return -EAGAIN;
		}
	}
	/*
	 * Per SD spec (section "Timing to Switch Signal Voltage"),
	 * host must gate clock at least 5ms.
	 */
	sd_clock = card->bus_io.clock;
	card->bus_io.clock = 0;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		pr_err("Failed to gate SD clock\n");
		return ret;
	}
	/* Now that clock is gated, change signal voltage */
	card->bus_io.signal_voltage = SD_VOL_1_8_V;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		pr_err("Failed to switch SD host to 1.8V\n");
		return ret;
	}
	sd_delay(10); /* Gate for 10ms, even though spec requires 5 */
	/* Restart the clock */
	card->bus_io.clock = sd_clock;
	ret = sdhc_set_io(card->sdhc, &card->bus_io);
	if (ret) {
		pr_err("Failed to restart SD clock\n");
		return ret;
	}
	/*
	 * If SD does not drive at least one of
	 * DAT[3:0] high within 1ms, switch failed
	 */
	sd_delay(1);
	if (sdhc_card_busy(card->sdhc)) {
		pr_err("Card failed to switch voltages\n");
		return -EAGAIN;
	}
	card->card_voltage = SD_VOL_1_8_V;
	pr_info("Card switched to 1.8V signaling\n");
	return 0;
}

/*
 * Requests card to publish a new relative card address, and move from
 * identification to data mode
 */
int sdmmc_request_rca(struct sd_card *card) {
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = SD_SEND_RELATIVE_ADDR;
	cmd.arg = 0;
	cmd.response_type = SD_RSP_TYPE_R6;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	/* Issue CMD3 until card responds with nonzero RCA */
	do {
		ret = sdhc_request(card->sdhc, &cmd, NULL);
		if (ret) {
			pr_err("CMD3 failed\n");
			return ret;
		}
		/* Card RCA is in upper 16 bits of response */
		card->relative_addr = ((cmd.response[0U] & 0xFFFF0000) >> 16U);
	} while (card->relative_addr == 0U);
	pr_dbg("Card relative addr: %d\n", card->relative_addr);
	return 0;
}

/*
 * Selects card, moving it into data transfer mode
 */
int sdmmc_select_card(struct sd_card *card) {
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = SD_SELECT_CARD;
	cmd.arg = ((card->relative_addr) << 16U);
	cmd.response_type = SD_RSP_TYPE_R1;
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		pr_err("CMD7 failed\n");
		return ret;
	}
	ret = sd_check_response(&cmd);
	if (ret) {
		pr_err("CMD7 reports error\n");
		return ret;
	}
	return 0;
}

/* Helper to send SD app command */
int card_app_command(struct sd_card *card, int relative_card_address) {
	struct sdhc_command cmd;
	int ret;

	cmd.opcode = SD_APP_CMD;
	cmd.arg = relative_card_address << 16U;
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;
	ret = sdhc_request(card->sdhc, &cmd, NULL);
	if (ret) {
		/* We want to retry transmission */
		return SD_RETRY;
	}
	ret = sd_check_response(&cmd);
	if (ret) {
		pr_warn("SD app command failed with R1 response of 0x%X\n", cmd.response[0]);
		return -EIO;
	}
	/* Check application command flag to determine if card is ready for APP CMD */
	if ((!card->host_props.is_spi) && !(cmd.response[0U] & SD_R1_APP_CMD)) {
		/* Command succeeded, but card not ready for app command. No APP CMD support */
		return -ENOTSUP;
	}
	return 0;
}

static int card_read(struct sd_card *card, uint8_t *rbuf, uint32_t start_block,
					 uint32_t num_blocks) {
	int ret;
	struct sdhc_command cmd;
	struct sdhc_data data;

	/*
	 * Note: The SD specification allows for CMD23 to be sent before a
	 * transfer in order to set the block length (often preferable).
	 * The specification also requires that CMD12 be sent to stop a transfer.
	 * However, the host specification defines support for "Auto CMD23" and
	 * "Auto CMD12", where the host sends CMD23 and CMD12 automatically to
	 * remove the overhead of interrupts in software from sending these
	 * commands. Therefore, we will not handle CMD12 or CMD23 at this layer.
	 * The host SDHC driver is expected to recognize CMD17, CMD18, CMD24,
	 * and CMD25 as special read/write commands and handle CMD23 and
	 * CMD12 appropriately.
	 */
	cmd.opcode = (num_blocks == 1U) ? SD_READ_SINGLE_BLOCK : SD_READ_MULTIPLE_BLOCK;
	if (!(card->flags & SD_HIGH_CAPACITY_FLAG)) {
		/* SDSC cards require block size in bytes, not blocks */
		cmd.arg = start_block * card->block_size;
	} else {
		cmd.arg = start_block;
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.retries = CONFIG_SD_DATA_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_addr = start_block;
	data.block_size = card->block_size;
	data.blocks = num_blocks;
	data.data = rbuf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	pr_dbg("READ: Sector = %u, Count = %u\n", start_block, num_blocks);

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		pr_err("Failed to read from SDMMC %d\n", ret);
		return ret;
	}

	/* Verify card is back in transfer state after read */
	ret = sdmmc_wait_ready(card);
	if (ret) {
		pr_err("Card did not return to ready state\n");
		return -ETIMEDOUT;
	}
	return 0;
}

/* Reads data from SD card memory card */
int card_read_blocks(struct sd_card *card, uint8_t *rbuf, uint32_t start_block,
					 uint32_t num_blocks) {
	int ret;
	uint32_t rlen;
	uint32_t sector;
	uint8_t *buf_offset;

	if ((start_block + num_blocks) > card->block_count) {
		return -EINVAL;
	}
	if (card->type == CARD_SDIO) {
		pr_warn("SDIO does not support MMC commands\n");
		return -ENOTSUP;
	}
	ret = tx_mutex_get(&card->lock, TX_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		pr_warn("Could not get SD card mutex\n");
		return -EBUSY;
	}

	/*
	 * If the buffer we are provided with is aligned, we can use it
	 * directly. Otherwise, we need to use the card's internal buffer
	 * and memcpy the data back out
	 */
	if ((((uintptr_t)rbuf) & (CONFIG_SDHC_BUFFER_ALIGNMENT - 1)) != 0) {
		/* lower bits of address are set, not aligned. Use internal buffer */
		pr_dbg("Unaligned buffer access to SD card may incur performance penalty\n");
		if (sizeof(card->card_buffer) < card->block_size) {
			pr_err("Card buffer size needs to be increased for "
				   "unaligned writes to work\n");
			tx_mutex_put(&card->lock);
			return -ENOBUFS;
		}
		rlen = sizeof(card->card_buffer) / card->block_size;
		sector = 0;
		buf_offset = rbuf;
		while (sector < num_blocks) {
			/* Read from disk to card buffer */
			ret = card_read(card, card->card_buffer, sector + start_block, rlen);
			if (ret) {
				pr_err("Write failed\n");
				tx_mutex_put(&card->lock);
				return ret;
			}
			/* Copy data from card buffer */
			memcpy(buf_offset, card->card_buffer, rlen * card->block_size);
			/* Increase sector count and buffer offset */
			sector += rlen;
			buf_offset += rlen * card->block_size;
		}
	} else {
		/* Aligned buffers can be used directly */
		ret = card_read(card, rbuf, start_block, num_blocks);
		if (ret) {
			pr_err("Card read failed\n");
			tx_mutex_put(&card->lock);
			return ret;
		}
	}
	tx_mutex_put(&card->lock);
	return 0;
}

/*
 * Sends ACMD22 (number of written blocks) to see how many blocks were written
 * to a card
 */
static int card_query_written(struct sd_card *card, uint32_t *num_written) {
	int ret;
	struct sdhc_command cmd;
	struct sdhc_data data;
	uint32_t *blocks = (uint32_t *)card->card_buffer;

	ret = card_app_command(card, card->relative_addr);
	if (ret) {
		pr_err("App CMD for ACMD22 failed\n");
		return ret;
	}

	cmd.opcode = SD_APP_SEND_NUM_WRITTEN_BLK;
	cmd.arg = 0;
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.retries = CONFIG_SD_CMD_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_addr = 0; /* Unused set to 0 */
	data.block_size = 4U;
	data.blocks = 1U;
	data.data = blocks;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		pr_err("ACMD22 failed: %d\n", ret);
		return ret;
	}
	ret = sd_check_response(&cmd);
	if (ret) {
		pr_err("ACMD22 reports error\n");
		return ret;
	}

	/* Decode blocks */
	*num_written = rte_be32_to_cpu(blocks[0]);
	return 0;
}

static int card_write(struct sd_card *card, const uint8_t *wbuf, uint32_t start_block,
					  uint32_t num_blocks) {
	int ret;
	uint32_t blocks;
	struct sdhc_command cmd;
	struct sdhc_data data;

	/*
	 * See the note in card_read() above. We will not issue CMD23
	 * or CMD12, and expect the host to handle those details.
	 */
	cmd.opcode = (num_blocks == 1) ? SD_WRITE_SINGLE_BLOCK : SD_WRITE_MULTIPLE_BLOCK;
	if (!(card->flags & SD_HIGH_CAPACITY_FLAG)) {
		/* SDSC cards require block size in bytes, not blocks */
		cmd.arg = start_block * card->block_size;
	} else {
		cmd.arg = start_block;
	}
	cmd.response_type = (SD_RSP_TYPE_R1 | SD_SPI_RSP_TYPE_R1);
	cmd.retries = CONFIG_SD_DATA_RETRIES;
	cmd.timeout_ms = CONFIG_SD_CMD_TIMEOUT;

	data.block_addr = start_block;
	data.block_size = card->block_size;
	data.blocks = num_blocks;
	data.data = (uint8_t *)wbuf;
	data.timeout_ms = CONFIG_SD_DATA_TIMEOUT;

	pr_dbg("WRITE: Sector = %u, Count = %u\n", start_block, num_blocks);

	ret = sdhc_request(card->sdhc, &cmd, &data);
	if (ret) {
		pr_err("Write failed: %d\n", ret);
		ret = sdmmc_wait_ready(card);
		if (ret) {
			return ret;
		}
		/* Query card to see how many blocks were actually written */
		ret = card_query_written(card, &blocks);
		if (ret) {
			return ret;
		}
		pr_err("Only %d blocks of %d were written\n", blocks, num_blocks);
		return -EIO;
	}
	/* Verify card is back in transfer state after write */
	ret = sdmmc_wait_ready(card);
	if (ret) {
		pr_err("Card did not return to ready state\n");
		return -ETIMEDOUT;
	}
	return 0;
}

/* Writes data to SD card memory card */
int card_write_blocks(struct sd_card *card, const uint8_t *wbuf, uint32_t start_block,
					  uint32_t num_blocks) {
	int ret;
	uint32_t wlen;
	uint32_t sector;
	const uint8_t *buf_offset;

	if ((start_block + num_blocks) > card->block_count) {
		return -EINVAL;
	}
	if (card->type == CARD_SDIO) {
		pr_warn("SDIO does not support MMC commands\n");
		return -ENOTSUP;
	}
	ret = tx_mutex_get(&card->lock, TX_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		pr_warn("Could not get SD card mutex\n");
		return -EBUSY;
	}
	/*
	 * If the buffer we are provided with is aligned, we can use it
	 * directly. Otherwise, we need to use the card's internal buffer
	 * and memcpy the data back out
	 */
	if ((((uintptr_t)wbuf) & (CONFIG_SDHC_BUFFER_ALIGNMENT - 1)) != 0) {
		/* lower bits of address are set, not aligned. Use internal buffer */
		pr_dbg("Unaligned buffer access to SD card may incur performance penalty\n");
		if (sizeof(card->card_buffer) < card->block_size) {
			pr_err("Card buffer size needs to be increased for "
				   "unaligned writes to work\n");
			tx_mutex_put(&card->lock);
			return -ENOBUFS;
		}
		wlen = sizeof(card->card_buffer) / card->block_size;
		sector = 0;
		buf_offset = wbuf;
		while (sector < num_blocks) {
			/* Copy data into card buffer */
			memcpy(card->card_buffer, buf_offset, wlen * card->block_size);
			/* Write card buffer to disk */
			ret = card_write(card, card->card_buffer, sector + start_block, wlen);
			if (ret) {
				pr_err("Write failed");
				tx_mutex_put(&card->lock);
				return ret;
			}
			/* Increase sector count and buffer offset */
			sector += wlen;
			buf_offset += wlen * card->block_size;
		}
	} else {
		/* We can use aligned buffers directly */
		ret = card_write(card, wbuf, start_block, num_blocks);
		if (ret) {
			pr_err("Write failed");
			tx_mutex_put(&card->lock);
			return ret;
		}
	}
	tx_mutex_put(&card->lock);
	return 0;
}

/* IO Control handler for SD MMC */
int card_ioctl(struct sd_card *card, uint8_t cmd, void *buf) {
	int ret;

	ret = tx_mutex_get(&card->lock, TX_MSEC(CONFIG_SD_DATA_TIMEOUT));
	if (ret) {
		pr_warn("Could not get SD card mutex");
		return ret;
	}
	switch (cmd) {
	case BLKDEV_IOC_GET_BLKCOUNT:
		(*(uint32_t *)buf) = card->block_count;
		break;
	case BLKDEV_IOC_GET_BLKSIZE:
	case BLKDEV_IOC_GET_ERASE_BLKSIZE:
		(*(uint32_t *)buf) = card->block_size;
		break;
	case BLKDEV_IOC_SYNC:
		/* Ensure card is not busy with data write.
		 * Note that SD stack does not support enabling caching, so
		 * cache flush is not required here
		 */
		ret = sdmmc_wait_ready(card);
		break;
	default:
		ret = -ENOTSUP;
	}
	tx_mutex_put(&card->lock);
	return ret;

	return -ENOSYS;
}
