/*
 * Copyright (c) 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include "tx_api.h"

#include "base/bitops.h"
#include "stm32_dma.h"


static const uint8_t dma_irqs[] = {
    DMA1_Stream0_IRQn,
    DMA1_Stream1_IRQn,
    DMA1_Stream2_IRQn,
    DMA1_Stream3_IRQn,
    DMA1_Stream4_IRQn,
    DMA1_Stream5_IRQn,
    DMA1_Stream6_IRQn,
    DMA1_Stream7_IRQn,

    DMA2_Stream0_IRQn,
    DMA2_Stream1_IRQn,
    DMA2_Stream2_IRQn,
    DMA2_Stream3_IRQn,
    DMA2_Stream4_IRQn,
    DMA2_Stream5_IRQn,
    DMA2_Stream6_IRQn,
    DMA2_Stream7_IRQn,
};
static TX_MUTEX dma_mtx;
static uint16_t dma_channels = 0xFFFF;

int stm32_dma_request(struct stm32_dmachan *chan, void (*dma_isr)(void *), void *arg) {
    if (chan == NULL)
        return -EINVAL;

    guard(os_mutex)(&dma_mtx);

    if (dma_channels == 0)
        return -ENODATA;
    
    int channel = ffs(dma_channels) - 1;
    dma_channels &= ~BIT(channel);

    if (dma_isr) {
        int err = request_irq(dma_irqs[channel], dma_isr, arg);
        if (err) {
            printk("failed to request dma interrupt (%d)\n", err);
            dma_channels &= ~BIT(channel);
            return err;
        }
    }

    if (channel < 8) {
        __HAL_RCC_DMA1_CLK_ENABLE();
        chan->ch  = (uint16_t)channel;
        chan->dma = DMA1;
    } else {
        __HAL_RCC_DMA2_CLK_ENABLE();
        chan->ch  = (uint16_t)channel - 8;
        chan->dma = DMA2;
    }

    return 0;
}

int stm32_dma_release(struct stm32_dmachan *chan) {
    if (chan == NULL || chan->ch >= 8)
        return -EINVAL;

    if (chan->dma != (void *)DMA1 && chan->dma != (void *)DMA2)
        return -EINVAL;

    guard(os_mutex)(&dma_mtx);
    int channel;

    if (chan->dma != (void *)DMA1)
        channel = chan->ch;
    else if (chan->dma != (void *)DMA2)
        channel = chan->ch + 8;
    else
        return -EINVAL;

    if (dma_channels & BIT(channel)) {
        dma_channels &= ~BIT(channel);
        disable_irq(dma_irqs[channel]);
        chan->dma = NULL;
    }

    return 0;
}

static int stm32_dma_init(void) {
    tx_mutex_create(&dma_mtx, "dma", TX_INHERIT);
    return 0;
}

SYSINIT(stm32_dma_init, SI_MEMORY_LEVEL, 10);
