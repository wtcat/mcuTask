/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define pr_fmt(fmt) "[sdmmc]: "fmt
// #define CONFIG_LOGLEVEL   LOGLEVEL_DEBUG

#include <errno.h>
#include <inttypes.h>

#include <tx_api.h>
#include <basework/assert.h>
#include <basework/log.h>
#include <basework/assert.h>
#include <drivers/sdio/mmcsd_core.h>

#include <subsys/cli/cli.h>

struct stm32_sdmmc_config {
    uint32_t fmax;
    int irq;
    uint32_t clken;
    void (*clk_enable)(uint32_t);
};

struct stm32_sdmmc {
    struct mmcsd_host host;
    SDMMC_TypeDef *reg;
    TX_SEMAPHORE sdio_idle;
    volatile uint32_t status;
    uint32_t mclk;
    const struct stm32_sdmmc_config *config;
};

// #define STM32_SDMMC_DEBUG

#define to_sdmmc(_dev) rte_container_of(_dev, struct stm32_sdmmc, host)

#define SDIO_BUFF_SIZE 2048
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
static void stm32_sdmmc_dumpreg(SDMMC_TypeDef *sdmmc,
    struct sdhc_command *cmd, struct sdhc_data *data) {
    printk("\n\n@send command: %ld (data: %p)\n", cmd->opcode, data);
    // printk("SDMMC BASE: %p\n", sdmmc);
    printk("\tPOWER = 0x%lx\n", sdmmc->POWER);
    printk("\tCLKCR = 0x%lx\n", sdmmc->CLKCR);
    printk("\tARG = 0x%lx\n", sdmmc->ARG);
    printk("\tCMD = 0x%lx\n", sdmmc->CMD);
    // printk("\tRESPCMD = 0x%lx\n", sdmmc->RESPCMD);
    // printk("\tRESP1 = 0x%lx\n", sdmmc->RESP1);
    // printk("\tRESP2 = 0x%lx\n", sdmmc->RESP2);
    // printk("\tRESP3 = 0x%lx\n", sdmmc->RESP3);
    // printk("\tRESP4 = 0x%lx\n", sdmmc->RESP4);
    // printk("\tDTIMER = 0x%lx\n", sdmmc->DTIMER);
    printk("\tDLEN = 0x%lx\n", sdmmc->DLEN);
    printk("\tDCTRL = 0x%lx\n", sdmmc->DCTRL);
    printk("\tDCOUNT = 0x%lx\n", sdmmc->DCOUNT);
    printk("\tSTA = 0x%lx\n", sdmmc->STA);
    printk("\tICR = 0x%lx\n", sdmmc->ICR);
    printk("\tMASK = 0x%lx\n", sdmmc->MASK);
    // printk("\tACKTIME = 0x%lx\n", sdmmc->ACKTIME);
    printk("\tIDMACTRL = 0x%lx\n", sdmmc->IDMACTRL);
    printk("\tIDMABSIZE = 0x%lx\n", sdmmc->IDMABSIZE);
    printk("\tIDMABASE0 = 0x%lx\n", sdmmc->IDMABASE0);
    // printk("\tIDMABASE1 = 0x%lx\n", sdmmc->IDMABASE1);
    // printk("\tFIFO = 0x%lx\n", sdmmc->FIFO);
    // printk("\tIPVR = 0x%lx\n\n", sdmmc->IPVR);
}

static int stm32_hrtimer_cmd(struct cli_process *cli, int argc, 
    char *argv[]) {
    (void) cli;
    (void) argc;
    (void) argv;
    stm32_sdmmc_dumpreg(SDMMC1, NULL, NULL);
	return 0;
}

CLI_CMD(sdmmc, "sdmmc",
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

    tx_semaphore_ceiling_put(&sd->sdio_idle, 1);
}

static void 
stm32_sdmmc_wait_complete(struct stm32_sdmmc *sd, struct mmcsd_cmd *cmd) {
    SDMMC_TypeDef *reg = sd->reg;
    UINT err;

    err = tx_semaphore_get(&sd->sdio_idle, TX_MSEC(5000));
    if (err) {
        printk("wait for sdio complete timeout\n");
        cmd->err = -ETIMEDOUT;
        return;
    }

    cmd->resp[0] = reg->RESP1;
    if (resp_type(cmd) == RESP_R2) {
        cmd->resp[1] = reg->RESP2;
        cmd->resp[2] = reg->RESP3;
        cmd->resp[3] = reg->RESP4;
    }

    uint32_t status = sd->status;
    if (status & SDIO_ERRORS) {
        if ((status & SDMMC_STA_CCRCFAIL) && 
            (resp_type(cmd) & (RESP_R3 | RESP_R4)))
            return;
        
        printk("sdio bus error(status = 0x%08"PRIx32")\n", status);
        cmd->err = -EIO;
    }
}

static int 
stm32_sdmmc_sendcmd(struct stm32_sdmmc *sd, struct mmcsd_cmd *cmd, 
    struct mmcsd_data *data) {
    static uint8_t dma_buffer[SDIO_BUFF_SIZE] __rte_aligned(RTE_CACHE_LINE_SIZE);
    SDMMC_TypeDef *reg = sd->reg;
    void *pbuffer = NULL;
    uint32_t regcmd;
    uint32_t bytes = 0;

    pr_dbg(" CMD:%ld ARG:0x%08lx RES:%s%s%s%s%s%s%s%s%s rw:%c len:%ld blksize:%ld\n",
          cmd->cmd_code,
          cmd->arg,
          resp_type(cmd) == RESP_NONE ? "NONE"  : "",
          resp_type(cmd) == RESP_R1  ? "R1"  : "",
          resp_type(cmd) == RESP_R1B ? "R1B"  : "",
          resp_type(cmd) == RESP_R2  ? "R2"  : "",
          resp_type(cmd) == RESP_R3  ? "R3"  : "",
          resp_type(cmd) == RESP_R4  ? "R4"  : "",
          resp_type(cmd) == RESP_R5  ? "R5"  : "",
          resp_type(cmd) == RESP_R6  ? "R6"  : "",
          resp_type(cmd) == RESP_R7  ? "R7"  : "",
          data ? (data->flags & DATA_DIR_WRITE ?  'w' : 'r') : '-',
          data ? data->blks * data->blksize : 0,
          data ? data->blksize : 0
         );

    reg->CMD = 0;
    reg->MASK |= SDIO_MASKR_ALL;
    regcmd = cmd->cmd_code | SDMMC_CMD_CPSMEN;

    /* data pre configuration */
    if (data != NULL) {
        bytes = data->blks * data->blksize;
        rte_assert(bytes <= SDIO_BUFF_SIZE);
        pbuffer = data->buf;
        if ((uintptr_t)pbuffer & (RTE_CACHE_LINE_SIZE - 1)) {
            pbuffer = dma_buffer;
            if (data->flags & DATA_DIR_WRITE)
                memcpy(pbuffer, data->buf, bytes);
        }
    
        if (data->flags & DATA_DIR_WRITE)
            SCB_CleanDCache_by_Addr(pbuffer, bytes);
        else
            SCB_InvalidateDCache_by_Addr(pbuffer, bytes);
        regcmd |= SDMMC_CMD_CMDTRANS;
        reg->MASK &= ~(SDMMC_MASK_CMDRENDIE | SDMMC_MASK_CMDSENTIE);
        reg->DTIMER = UINT32_MAX;
        reg->DLEN = bytes;
        reg->DCTRL = ((ffs(data->blksize) - 1) << 4) 
                | ((data->flags & DATA_DIR_READ) ? SDMMC_DCTRL_DTDIR : 0);
        reg->IDMABASE0 = (uint32_t)pbuffer;
        reg->IDMACTRL = SDMMC_IDMA_IDMAEN;
    }

    if (resp_type(cmd) == RESP_R2)
        regcmd |= SDMMC_CMD_WAITRESP;
    else if (resp_type(cmd) != RESP_NONE)
        regcmd |= SDMMC_CMD_WAITRESP_0;

    reg->ARG = cmd->arg;
    reg->CMD = regcmd;

    /* Wait for sdio transfer to complete */
    stm32_sdmmc_wait_complete(sd, cmd);

    if (data != NULL) {
        int timeout = 5;
        while (reg->STA & SDMMC_STA_DPSMACT) {
            if (--timeout < 0) {
                cmd->err = -EIO;
                return -1;
            }
            tx_thread_sleep(TX_MSEC(1));
        }

        if (pbuffer == dma_buffer && (data->flags & DATA_DIR_READ)) 
            memcpy(data->buf, pbuffer, bytes);
    }

    return 0;
}

static void 
stm32_sdmmc_request(struct mmcsd_host *host, struct mmcsd_req *req) {
    struct stm32_sdmmc *sd = to_sdmmc(host);

    if (req->cmd)
        stm32_sdmmc_sendcmd(sd, req->cmd, req->data);

    if (req->stop)
        stm32_sdmmc_sendcmd(sd, req->stop, NULL);

    mmcsd_req_complete(&sd->host);
}

static void 
stm32_sdmmc_iocfg(struct mmcsd_host *host, struct mmcsd_io_cfg *cfg) {
    struct stm32_sdmmc *sd = to_sdmmc(host);
    uint32_t clk = cfg->clock;
    uint32_t temp = 0;

    if (clk > 0) {
        if (clk > host->freq_max)
            clk = host->freq_max;
        temp = rte_div_roundup(sd->mclk, 2 * clk);
        if (temp > 0x3FF)
            temp = 0x3FF;
    }

    if (cfg->bus_width == MMCSD_BUS_WIDTH_4)
        temp |= SDMMC_CLKCR_WIDBUS_0;
    else if (cfg->bus_width == MMCSD_BUS_WIDTH_8)
        temp |= SDMMC_CLKCR_WIDBUS_1;

    sd->reg->CLKCR = temp;
    if (cfg->power_mode == MMCSD_POWER_ON)
        sd->reg->POWER |= SDMMC_POWER_PWRCTRL;
}

static int _stm32_sdmmc_init(struct stm32_sdmmc *sd) {
    const struct stm32_sdmmc_config *config = sd->config;
    struct mmcsd_host *host = &sd->host;
    int err;

    mmcsd_host_init(&sd->host);
    err = request_irq(config->irq, stm32_sdmmc_isr, sd);
    if (err) {
        pr_err("failed to request irq(%d)\n", config->irq);
        goto _remove_host;
    }

    tx_semaphore_create(&sd->sdio_idle, (CHAR *)"sdio", 0);

    /* Enable periph clock */
    config->clk_enable(config->clken);
    sd->reg->MASK = 0;
    sd->reg->ICR = 0xFFFFFFFF;
    sd->mclk = LL_RCC_GetSDMMCClockFreq(LL_RCC_SDMMC_CLKSOURCE);

    /* set host default attributes */
    host->ops.request = stm32_sdmmc_request;
    host->ops.set_iocfg = stm32_sdmmc_iocfg;

    host->freq_min = 400 * 1000;
    host->freq_max = sd->config->fmax;
    host->valid_ocr = VDD_32_33 | VDD_33_34;
    host->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_HIGHSPEED;
    host->max_seg_size  = SDIO_BUFF_SIZE;
    host->max_dma_segs  = 1;
    host->max_blk_size  = 512;
    host->max_blk_count = 512;
    mmcsd_change(host);

    printk("register sdmmc0 device\n");
    return 0;

_remove_host:
    mmcsd_host_uninit(&sd->host);
    return err;
}

static const struct stm32_sdmmc_config sdmmc1_config = {
    .fmax = 50000000,
    .irq  = SDMMC1_IRQn,
    .clken = RCC_AHB3ENR_SDMMC1EN,
    .clk_enable = LL_AHB3_GRP1_EnableClock
};
static struct stm32_sdmmc sdmmc1_device = {
    .host = {
        .name = "sdmmc1",
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
