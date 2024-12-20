/*
 * Copyright (c) 2024 wtcat
 */

#include "tx_api.h"
#include "subsys/sd/sdhc.h"

#include "stm32h7xx_ll_sdmmc.h"

struct stm32_sdmmc {
    struct sdhc_device dev;
    SDMMC_TypeDef *reg;
};

#define to_sdmmc(dev) (struct stm32_sdmmc *)(dev)


static int stm32_sdmmc_reset(struct device *dev) {

}

static int stm32_sdmmc_request(struct device *dev, struct sdhc_command *cmd,
    struct sdhc_data *data) {

}

static int stm32_sdmmc_set_io(struct device *dev, struct sdhc_io *ios) {

}

static int stm32_sdmmc_get_card_present(struct device *dev) {

}

static int stm32_sdmmc_execute_tuning(struct device *dev) {

}

static int stm32_sdmmc_card_busy(struct device *dev) {

}

static int stm32_sdmmc_get_host_props(struct device *dev, 
    struct sdhc_host_props *props) {

}

static int stm32_sdmmc_enable_interrupt(struct device *dev, 
    sdhc_interrupt_cb_t callback, int sources, void *user_data) {             

}

static int stm32_sdmmc_disable_interrupt(struct device *dev, int sources) {

}
