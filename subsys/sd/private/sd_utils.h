/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Common utility functions for SD subsystem
 */

#ifndef SUBSYS_SD_SD_UTILS_H_
#define SUBSYS_SD_SD_UTILS_H_

#include "tx_api.h"
#include "subsys/sd/sd.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_SDHC_SUPPORTS_NATIVE_MODE 1
#define CONFIG_SDHC_SUPPORTS_SPI_MODE 1
#define CONFIG_SD_UHS_PROTOCOL 1

#define CONFIG_SDIO_STACK 1
#define CONFIG_SDMMC_STACK 1
#define CONFIG_MMC_STACK 1

/* Number of times to retry sending command to card */
#ifndef CONFIG_SD_CMD_RETRIES
#define CONFIG_SD_CMD_RETRIES 0
#endif

/* Timeout for SD commands (in ms) */
#ifndef CONFIG_SD_CMD_TIMEOUT
#define CONFIG_SD_CMD_TIMEOUT 200
#endif

/* Number of times to retry sending data to card */
#ifndef CONFIG_SD_DATA_RETRIES
#define CONFIG_SD_DATA_RETRIES 3
#endif

/* Timeout for SD data transfer (in ms) */
#ifndef CONFIG_SD_DATA_TIMEOUT
#define CONFIG_SD_DATA_TIMEOUT 1000
#endif

/* Number of times to retry initialization commands */
#ifndef CONFIG_SD_RETRY_COUNT
#define CONFIG_SD_RETRY_COUNT 10
#endif

/* Timeout while initializing SD card */
#ifndef CONFIG_SD_INIT_TIMEOUT
#define CONFIG_SD_INIT_TIMEOUT 1500
#endif

/* Number of times to retry SD OCR read */
#ifndef CONFIG_SD_OCR_RETRY_COUNT
#define CONFIG_SD_OCR_RETRY_COUNT 1000
#endif

/* MMC Relative card address */
#ifndef CONFIG_MMC_RCA
#define CONFIG_MMC_RCA 2
#endif

/**
 * Custom SD return codes. Used internally to indicate conditions that may
 * not be errors, but are abnormal return conditions
 */
enum sd_return_codes {
	SD_RETRY = 1,
	SD_NOT_SDIO = 2,
	SD_RESTART = 3,
};

/* Checks SD status return codes */
static inline int sd_check_response(struct sdhc_command *cmd) {
	if (cmd->response_type == SD_RSP_TYPE_R1) {
		return (cmd->response[0U] & SD_R1_ERR_FLAGS);
	}
	return 0;
}

/* Delay function for SD subsystem */
static inline void sd_delay(unsigned int millis) {
	tx_thread_sleep(TX_MSEC(millis));
}

/*
 * Helper function to retry sending command to SD card
 * Will retry command if return code equals SD_RETRY
 */
static inline int sd_retry(int (*cmd)(struct sd_card *card), struct sd_card *card,
						   int retries) {
	int ret = -ETIMEDOUT;

	while (retries-- >= 0) {
		/* Try cmd */
		ret = cmd(card);
		/**
		 * Functions have 3 possible responses:
		 * 0: success
		 * SD_RETRY: retry command
		 * other: does not retry
		 */
		if (ret != SD_RETRY) {
			break;
		}
	}
	return ret == SD_RETRY ? -ETIMEDOUT : ret;
}

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_SD_SD_UTILS_H_ */
