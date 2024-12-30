/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define pr_fmt(fmt) "<sdmmc>: "
#define CONFIG_LOGLEVEL   LOGLEVEL_DEBUG
#include <errno.h>
#include <inttypes.h>

#include "tx_api.h"
#include "basework/assert.h"
#include "basework/log.h"

#include "subsys/sd/sdhc.h"
#include "subsys/sd/sd.h"
#include "subsys/cli/cli.h"

struct stm32_sdmmc_config {
    uint32_t fmax;
    int irq;
    uint32_t clken;
    void (*clk_enable)(uint32_t);
};

struct stm32_sdmmc {
    struct sd_card card;
    struct sdhc_device dev;
    SDMMC_TypeDef *reg;
    TX_SEMAPHORE reqdone;
    uint32_t status;
    uint32_t mclk;
    const struct stm32_sdmmc_config *config;
};

#define STM32_SDMMC_DEBUG
#define to_sdmmc(_dev) rte_container_of(_dev, struct stm32_sdmmc, dev)

#define SDIO_ERRORS \
    (SDMMC_STA_IDMATE | SDMMC_STA_ACKTIMEOUT | \
     SDMMC_STA_RXOVERR | SDMMC_STA_TXUNDERR | \
     SDMMC_STA_DTIMEOUT | SDMMC_STA_CTIMEOUT | \
     SDMMC_STA_DCRCFAIL | SDMMC_STA_CCRCFAIL)

#define SDIO_MASKR_ALL \
    (SDMMC_MASK_CCRCFAILIE | SDMMC_MASK_DCRCFAILIE | SDMMC_MASK_CTIMEOUTIE | \
     SDMMC_MASK_TXUNDERRIE | SDMMC_MASK_RXOVERRIE | SDMMC_MASK_CMDRENDIE | \
     SDMMC_MASK_CMDSENTIE | SDMMC_MASK_DATAENDIE | SDMMC_MASK_ACKTIMEOUTIE)

#ifdef STM32_SDMMC_DEBUG
static void stm32_sdmmc_dumpreg(SDMMC_TypeDef *sdmmc) {
    printk("SDMMC BASE: %p\n", sdmmc);
    printk("sdmmc->POWER = 0x%lx\n", sdmmc->POWER);
    printk("sdmmc->CLKCR = 0x%lx\n", sdmmc->CLKCR);
    printk("sdmmc->ARG = 0x%lx\n", sdmmc->ARG);
    printk("sdmmc->CMD = 0x%lx\n", sdmmc->CMD);
    printk("sdmmc->RESPCMD = 0x%lx\n", sdmmc->RESPCMD);
    printk("sdmmc->RESP1 = 0x%lx\n", sdmmc->RESP1);
    printk("sdmmc->RESP2 = 0x%lx\n", sdmmc->RESP2);
    printk("sdmmc->RESP3 = 0x%lx\n", sdmmc->RESP3);
    printk("sdmmc->RESP4 = 0x%lx\n", sdmmc->RESP4);
    printk("sdmmc->DTIMER = 0x%lx\n", sdmmc->DTIMER);
    printk("sdmmc->DLEN = 0x%lx\n", sdmmc->DLEN);
    printk("sdmmc->DCTRL = 0x%lx\n", sdmmc->DCTRL);
    printk("sdmmc->DCOUNT = 0x%lx\n", sdmmc->DCOUNT);
    printk("sdmmc->STA = 0x%lx\n", sdmmc->STA);
    printk("sdmmc->ICR = 0x%lx\n", sdmmc->ICR);
    printk("sdmmc->MASK = 0x%lx\n", sdmmc->MASK);
    printk("sdmmc->ACKTIME = 0x%lx\n", sdmmc->ACKTIME);
    printk("sdmmc->IDMACTRL = 0x%lx\n", sdmmc->IDMACTRL);
    printk("sdmmc->IDMABSIZE = 0x%lx\n", sdmmc->IDMABSIZE);
    printk("sdmmc->IDMABASE0 = 0x%lx\n", sdmmc->IDMABASE0);
    printk("sdmmc->IDMABASE1 = 0x%lx\n", sdmmc->IDMABASE1);
    printk("sdmmc->FIFO = 0x%lx\n", sdmmc->FIFO);
    printk("sdmmc->IPVR = 0x%lx\n\n", sdmmc->IPVR);
}

static int stm32_hrtimer_cmd(struct cli_process *cli, int argc, 
    char *argv[]) {
    (void) cli;
    (void) argc;
    (void) argv;
    stm32_sdmmc_dumpreg(SDMMC1);
	return 0;
}

CLI_CMD(sdmmc, "sdmmc_dump",
    "Show SDMMC register status",
    stm32_hrtimer_cmd
);
#endif /* STM32_SDMMC_DEBUG */

static void __fastcode 
stm32_sdmmc_isr(void *arg) {
    struct stm32_sdmmc *sd = arg;
    uint32_t sta = sd->reg->STA;

    sd->reg->ICR = sta;
    sd->status = sta;
    tx_semaphore_ceiling_put(&sd->reqdone, 1);
}

static int __fastcode 
stm32_sdmmc_sendcmd(struct stm32_sdmmc *sd, struct sdhc_command *cmd,
    struct sdhc_data *data) {
    SDMMC_TypeDef *reg = sd->reg;
    uint32_t regcmd;
    uint32_t resp;

    regcmd = cmd->opcode | SDMMC_CMD_CPSMEN;

    /* Reset request semaphore */
    tx_semaphore_get(&sd->reqdone, TX_NO_WAIT);

    if (data != NULL) {
        rte_assert(((uint32_t)data->data & (RTE_CACHE_LINE_SIZE - 1)) == 0);
        uint32_t timeout = (uint32_t)((uint64_t)sd->mclk * data->timeout_ms / 1000);
        uint32_t dctrl = (ffs(data->block_size) - 1) << SDMMC_DCTRL_DBLOCKSIZE_Pos;
        uint32_t dlen;

		switch (cmd->opcode) {
		case SD_WRITE_SINGLE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
			dctrl |= (SDMMC_TRANSFER_DIR_TO_CARD | SDMMC_TRANSFER_MODE_BLOCK);
			break;
		case SD_APP_SEND_SCR:
		case SD_SWITCH:
		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			dctrl |= (SDMMC_TRANSFER_DIR_TO_SDMMC | SDMMC_TRANSFER_MODE_BLOCK);
			break;
		default:
			return -ENOTSUP;
		}

        dlen = data->block_size * data->blocks;
        SCB_InvalidateDCache_by_Addr(data->data, dlen);
        reg->IDMABASE0 = (uint32_t)data->data;
        reg->DTIMER = timeout;
        reg->DLEN = dlen;
        reg->DCTRL = dctrl;
        reg->IDMACTRL = SDMMC_IDMA_IDMAEN;
        regcmd |= SDMMC_CMD_CMDTRANS;
    }

    resp = cmd->response_type & SDHC_NATIVE_RESPONSE_MASK;
    if (resp == SD_RSP_TYPE_R2)
        regcmd |= SDMMC_CMD_WAITRESP;
    else if (resp != SD_RSP_TYPE_NONE)
        regcmd |= SDMMC_CMD_WAITRESP_0;

    reg->MASK |= SDIO_MASKR_ALL;
    reg->ARG = cmd->arg;
    reg->CMD = regcmd;

    /* 
     * Waiting for transfer complete 
     */
    if (tx_semaphore_get(&sd->reqdone, TX_MSEC(3000))) {
        printk("sdmmc transfer timeout\n");
        return -ETIME;
    }

    cmd->response[0] = reg->RESP1;
    if (resp == SD_RSP_TYPE_R2) {
        cmd->response[1] = reg->RESP2;
        cmd->response[2] = reg->RESP3;
        cmd->response[3] = reg->RESP4;
    }

    if (sd->status & SDIO_ERRORS) {
        if (!((sd->status & SDMMC_STA_CCRCFAIL) && 
            (resp & (SD_RSP_TYPE_R3 | SD_RSP_TYPE_R4))))
            return -EIO;
    }

    return 0;
}

static int __fastcode 
stm32_sdmmc_request(struct device *dev, struct sdhc_command *cmd,
    struct sdhc_data *data) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);
    int err;

    err = stm32_sdmmc_sendcmd(sd, cmd, data);
    if (data != NULL) {
        if (sd->reg->STA & SDMMC_STA_DPSMACT) {
            //TODO;
            printk("dpsm busy!\n");
        }
    }

    return err;
}

static int stm32_sdmmc_set_io(struct device *dev, struct sdhc_io *ios) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);
    uint32_t clkcr = sd->reg->CLKCR;
    uint32_t clkrate = ios->clock;
    uint32_t div;

    /*
     * Configure clock rate and buswidth
     */
    clkcr &= ~CLKCR_CLEAR_MASK;
    if (ios->bus_width == SDHC_BUS_WIDTH1BIT)
        clkcr |= SDMMC_BUS_WIDE_1B;
    else if (ios->bus_width == SDHC_BUS_WIDTH4BIT)
        clkcr |= SDMMC_BUS_WIDE_4B;
    else if (ios->bus_width == SDHC_BUS_WIDTH8BIT)
        clkcr |= SDMMC_BUS_WIDE_8B;
    else
        clkcr |= SDMMC_BUS_WIDE_1B;

    if (clkrate > SD_CLOCK_50MHZ)
        clkrate = SD_CLOCK_50MHZ;
    
    div = rte_div_roundup(sd->mclk, (clkrate * 2));
    if (div >= 0x3FF)
        div = 0x3FF;

    clkcr |= div;
    clkcr |= SDMMC_HARDWARE_FLOW_CONTROL_ENABLE | SDMMC_CLOCK_EDGE_FALLING;
    sd->reg->CLKCR = clkcr;

    /*
     * Configure power state
     */
    if (ios->power_mode == SDHC_POWER_ON)
        sd->reg->POWER |= SDMMC_POWER_PWRCTRL;

    return 0;
}

static int stm32_sdmmc_get_card_present(struct device *dev) {
    (void) dev;
    return 1;
}

static int stm32_sdmmc_get_host_props(struct device *dev, 
    struct sdhc_host_props *props) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

    (void) sd;
    memset(props, 0, sizeof(*props));
    props->f_min = SDMMC_CLOCK_400KHZ;
    props->f_max = sd->config->fmax;
    props->host_caps.bus_4_bit_support = true;
    props->host_caps.vol_330_support = true;
    props->is_spi = false;

    return 0;
}

static int stm32_sdmmc_enable_interrupt(struct device *dev, 
    sdhc_interrupt_cb_t callback, int sources, void *user_data) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

    (void) sd;
    (void) callback;
    (void) sources;
    (void) user_data;
    return -ENOSYS;
}

static int stm32_sdmmc_disable_interrupt(struct device *dev, int sources) {
    struct stm32_sdmmc *sd = to_sdmmc(dev);

    (void) sd;
    (void) sources;
    return -ENOSYS;
}

static int _stm32_sdmmc_init(struct stm32_sdmmc *sd) {
    const struct stm32_sdmmc_config *config = sd->config;
    int err;

    tx_semaphore_create(&sd->reqdone, (CHAR *)sd->dev.name, 0);
    err = request_irq(config->irq, stm32_sdmmc_isr, sd);
    if (err) {
        pr_err("failed to request irq(%d)\n", config->irq);
        goto _remove_sem;
    }

    /* Enable periph clock */
    config->clk_enable(config->clken);
    sd->reg->MASK = 0;
    sd->reg->ICR = 0xFFFFFFFF;
    sd->mclk = LL_RCC_GetSDMMCClockFreq(LL_RCC_SDMMC_CLKSOURCE);

    pr_dbg("SDMMC bus clock: %"PRIu32"\n", sd->mclk);

    /* Register device */
    err = device_register((struct device *)sd);
    if (err) {
        pr_err("failed to reigister device %d\n", err);
        goto _remove_irq;
    }
    
    /* Initialize SD card */
    err = sd_init((struct device *)sd, &sd->card);
    if (err) {
        pr_err("failed to initialize sd %d\n", err);
        goto _remove_dev;
    }

    return 0;

_remove_dev:
    device_unregister((struct device *)sd);
_remove_irq:
    remove_irq(config->irq, stm32_sdmmc_isr, sd);
_remove_sem:
    tx_semaphore_delete(&sd->reqdone);
    return err;
}

static const struct stm32_sdmmc_config sdmmc1_config = {
    .fmax = SD_CLOCK_50MHZ,
    .irq = SDMMC1_IRQn,
    .clken = RCC_AHB3ENR_SDMMC1EN,
    .clk_enable = LL_AHB3_GRP1_EnableClock
};
static struct stm32_sdmmc sdmmc1_device = {
    .dev = {
        .name = "sdmmc1",
        .request = stm32_sdmmc_request,
        .set_io = stm32_sdmmc_set_io,
        .get_card_present = stm32_sdmmc_get_card_present,
        .get_host_props = stm32_sdmmc_get_host_props,
        .enable_interrupt = stm32_sdmmc_enable_interrupt,
        .disable_interrupt = stm32_sdmmc_disable_interrupt
    },
    .reg = SDMMC1,
    .config = &sdmmc1_config
};

static int stm32_sdmmc_init(void) {
    int err;

    err = _stm32_sdmmc_init(&sdmmc1_device);
    return err;
}

SYSINIT(stm32_sdmmc_init, SI_DRIVER_LEVEL, 10);
