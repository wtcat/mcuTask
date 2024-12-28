/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"
#include "subsys/sd/sdhc.h"
#include "subsys/sd/sd.h"

#include "stm32h7xx_ll_sdmmc.h"


struct stm32_sdmmc_config {
    int irq;
    uint32_t clken;
    void (*clk_enable)(uint32_t);
};

struct stm32_sdmmc {
    struct sdhc_device dev;
    SDMMC_TypeDef *reg;
    struct sd_card card;
    struct stm32_sdmmc_config *config;
};

#define to_sdmmc(dev) (struct stm32_sdmmc *)(dev)

static void stm32_sdmmc_isr(void *arg) {
    struct stm32_sdmmc *sdmmc = arg;

}

static int stm32_sdmmc_reset(struct device *dev) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);
}

static int stm32_sdmmc_request(struct device *dev, struct sdhc_command *cmd,
    struct sdhc_data *data) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);
}

static int stm32_sdmmc_set_io(struct device *dev, struct sdhc_io *ios) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);
    SDMMC_InitTypeDef sdcfg;

    sdcfg.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    sdcfg.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    sdcfg.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_ENABLE;

    if (ios->bus_width == SDHC_BUS_WIDTH1BIT)
        sdcfg.BusWide = SDMMC_BUS_WIDE_1B;
    else if (ios->bus_width == SDHC_BUS_WIDTH4BIT)
        sdcfg.BusWide = SDMMC_BUS_WIDE_4B;
    else if (ios->bus_width == SDHC_BUS_WIDTH8BIT)
        sdcfg.BusWide = SDMMC_BUS_WIDE_8B;
    else
        sdcfg.BusWide = SDMMC_BUS_WIDE_1B;

    if (ios->clock == SDMMC_CLOCK_400KHZ)
    else if ()

   
}

static int stm32_sdmmc_get_card_present(struct device *dev) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

}

static int stm32_sdmmc_execute_tuning(struct device *dev) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

}

static int stm32_sdmmc_card_busy(struct device *dev) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

}

static int stm32_sdmmc_get_host_props(struct device *dev, 
    struct sdhc_host_props *props) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

}

static int stm32_sdmmc_enable_interrupt(struct device *dev, 
    sdhc_interrupt_cb_t callback, int sources, void *user_data) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);            

}

static int stm32_sdmmc_disable_interrupt(struct device *dev, int sources) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

}

static int _stm32_sdmmc_init(struct stm32_sdmmc *sd) {
    const struct stm32_sdmmc_config *config = sd->config;
    int err;

    err = request_irq(config->irq, stm32_sdmmc_isr, sd);
    if (err)
        return err;

    /* Enable periph clock */
    config->clk_enable(config->clken);
    sd->reg->MASK = 0;
    sd->reg->ICR = 0xFFFFFFFF;

    /* Initialize SD card */
    return sd_init((struct device *)sd, &sd->card);
}


static const struct stm32_sdmmc_config sdmmc1_config = {
    .irq = SDMMC1_IRQn,
    .clken = RCC_AHB3ENR_SDMMC1EN,
    .clk_enable = LL_AHB3_GRP1_EnableClock
};
static struct stm32_sdmmc sdmmc1_device = {
    .dev = {
        .reset = stm32_sdmmc_reset,
        .request = stm32_sdmmc_request,
        .set_io = stm32_sdmmc_set_io,
        .get_card_present = stm32_sdmmc_get_card_present,
        .execute_tuning = stm32_sdmmc_execute_tuning,
        .card_busy = stm32_sdmmc_card_busy,
        .get_host_props = stm32_sdmmc_get_host_props,
        .enable_interrupt = stm32_sdmmc_enable_interrupt,
        .disable_interrupt = stm32_sdmmc_disable_interrupt
    },
    .reg = SDMMC1,
    .config = &sdmmc1_config
};

static int stm32_sdmmc_init(void) {
    return 0;
}

SYSINIT(stm32_sdmmc_init, SI_DRIVER_LEVEL, 10);
