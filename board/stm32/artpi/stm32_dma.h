/*
 * Copyright 2024 wtcat
 */
#ifndef STM32_DMA_H_
#define STM32_DMA_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif

struct stm32_dmastat {
#define STM32_DMA_FEIF    0x01
#define STM32_DMA_DMEIF   0x04
#define STM32_DMA_TEIF    0x08
#define STM32_DMA_HTIF    0x10
#define STM32_DMA_TCIF    0x20
#define STM32_DMA_ALLMASK 0x7D 
    volatile uint32_t isr[2];
    volatile uint32_t icr[2];
};

struct stm32_dmachan {
    int16_t ch;
    int16_t id;
};

#define stm32_dmachan_valid(_chan) ((_chan)->ch >= 0)

#define stm32_dmaisr_read(_sta, _chan) ({ \
    uint16_t __ch = (_chan)->ch; \
    uint32_t __sr = (_sta)->isr[__ch >> 2]; \
    __sr = __sr >> ((__ch & 3) << 3); \
    __sr; \
})

#define stm32_dmaicr_write(_sta, _chan, _mask) do {\
    uint16_t __ch = (_chan)->ch; \
    uint32_t __mask = (_mask) << ((__ch & 3) << 3); \
    (_sta)->icr[__ch >> 2] = __mask; \
} while (0)

#ifdef __cplusplus
}
#endif
#endif /* STM32_DMA_H_ */
