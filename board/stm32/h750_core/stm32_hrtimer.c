/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE
#define USE_HRTIMER_SOURCE_CODE
#define HRTIME_CONTEXT_EXTENSION \
    uint64_t jiffies; \
    uint32_t loadv;

#include "tx_api.h"
#include "basework/hrtimer_.h"

#include "stm32h7xx_ll_lptim.h"


/*
 * Timer parameter configure
 */
#define HR_TIMER         (LPTIM2)
#define HR_TIMER_IRQ     (LPTIM2_IRQn)
#define HR_TIMER_CLKEN() LL_APB4_GRP1_EnableClock(LL_APB4_GRP1_PERIPH_LPTIM2)
#define HR_TIMER_MAX     (UINT16_MAX)


static __fastdata HRTIMER_CONTEXT_DEFINE(hrtimer);

static void __fastcode 
load_next_timer(void) {
    if (hrtimer.first) {
        struct hrtimer *next_timer = _hrtimer_first(&hrtimer);
        uint64_t expire = next_timer->expire - hrtimer.jiffies;
        if (expire > (uint64_t)HR_TIMER_MAX)
            hrtimer.loadv = HR_TIMER_MAX;
        else
            hrtimer.loadv = expire;

        // HR_TIMER->CR1 &= ~0x1;
        // HR_TIMER->CNT = hrtimer.loadv;
        // HR_TIMER->CR1 |= 1;
    }
}

static void __fastcode 
stm32_timer_isr(void *arg) {
    TX_INTERRUPT_SAVE_AREA
    // uint32_t arr = HR_TIMER->ARR;

    (void) arg;
    if (HR_TIMER->ISR & LPTIM_ISR_ARROK) {
        HR_TIMER->ICR |= LPTIM_ICR_ARROKCF;

        TX_DISABLE
        struct hrtimer *timer = _hrtimer_first(&hrtimer);
        uint64_t now = hrtimer.jiffies + hrtimer.loadv;
        _hrtimer_expire((&hrtimer), timer, now, 
            TX_RESTORE
            routine(timer);
            TX_DISABLE
        );

        hrtimer.jiffies = now;
        load_next_timer();
        TX_RESTORE
    }

    printk("tim2\n");
}

void __fastcode 
hrtimer_init(struct hrtimer *timer) {
    memset(timer, 0, sizeof(*timer));
}

int __fastcode 
hrtimer_start(struct hrtimer *timer, uint64_t expire) {
    scoped_guard(os_irq) {
        hrtimer.jiffies += hrtimer.loadv - HR_TIMER->CNT;
        if (_hrtimer_insert(&hrtimer, timer, hrtimer.jiffies + expire)) {
            if (expire > (uint64_t)HR_TIMER_MAX)
                hrtimer.loadv = HR_TIMER_MAX;
            else
                hrtimer.loadv = expire;
            // HR_TIMER->CR1 &= ~0x1;
            // HR_TIMER->CNT = hrtimer.loadv;
            // HR_TIMER->CR1 |= 1;
        }
    }
    return 0;
}

int __fastcode 
hrtimer_stop(struct hrtimer *timer) {
    scoped_guard(os_irq) {
        if (_hrtimer_remove(&hrtimer, timer)) {
            hrtimer.jiffies += hrtimer.loadv - HR_TIMER->CNT;
            load_next_timer();
        }
    }
    return 0;
}

int stm32_hrtimer_init(void) {
    int err;

    err = request_irq(HR_TIMER_IRQ, stm32_timer_isr, NULL);
    if (!err) {
        HR_TIMER_CLKEN();
        LL_LPTIM_SetClockSource(HR_TIMER, LL_LPTIM_CLK_SOURCE_INTERNAL);
        LL_LPTIM_SetPrescaler(HR_TIMER, LL_LPTIM_PRESCALER_DIV8);
        LL_LPTIM_SetPolarity(HR_TIMER, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
        LL_LPTIM_SetUpdateMode(HR_TIMER, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
        LL_LPTIM_SetCounterMode(HR_TIMER, LL_LPTIM_COUNTER_MODE_INTERNAL);
        LL_LPTIM_DisableTimeout(HR_TIMER);

        /* counting start is initiated by software */
        LL_LPTIM_TrigSw(HR_TIMER);

        /* HR_TIMER interrupt set-up before enabling */
        LL_LPTIM_DisableIT_CMPM(HR_TIMER);
        LL_LPTIM_ClearFLAG_CMPM(HR_TIMER);

        /* Autoreload match Interrupt */
        LL_LPTIM_EnableIT_ARRM(HR_TIMER);
        LL_LPTIM_ClearFLAG_ARRM(HR_TIMER);

        /* ARROK bit validates the write operation to ARR register */
        LL_LPTIM_EnableIT_ARROK(HR_TIMER);
        LL_LPTIM_ClearFlag_ARROK(HR_TIMER);
    }

    return err;
}

SYSINIT(stm32_hrtimer_init, SI_PREDRIVER_LEVEL, 10);
