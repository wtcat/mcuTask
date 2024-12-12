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


/*
 * Timer parameter configure
 */
#define HR_TIMER         (TIM2)
#define HR_TIMER_IRQ     (TIM2_IRQn)
#define HR_TIMER_CLKEN() LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2)
#define HR_TIMER_MAX     (UINT32_MAX)


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

        HR_TIMER->CR1 &= ~0x1;
        HR_TIMER->CNT = hrtimer.loadv;
        HR_TIMER->CR1 |= 1;
    }
}

static void __fastcode 
stm32_timer_isr(void *arg) {
    TX_INTERRUPT_SAVE_AREA
    struct hrtimer *timer;
    uint64_t now;

    (void) arg;

    TX_DISABLE
    timer = _hrtimer_first(&hrtimer);
    now = hrtimer.jiffies + hrtimer.loadv;
    _hrtimer_expire((&hrtimer), timer, now, 
        TX_RESTORE
        routine(timer);
        TX_DISABLE
    );

    hrtimer.jiffies = now;
    load_next_timer();
    TX_RESTORE

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
            HR_TIMER->CR1 &= ~0x1;
            HR_TIMER->CNT = hrtimer.loadv;
            HR_TIMER->CR1 |= 1;
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
    LL_TIM_InitTypeDef TIM_InitStruct;

    LL_TIM_StructInit(&TIM_InitStruct);
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_DOWN;

    HR_TIMER_CLKEN();
    LL_TIM_Init(HR_TIMER, &TIM_InitStruct);
    LL_TIM_EnableIT_UPDATE(HR_TIMER);
    HR_TIMER->ARR = 0;
    HR_TIMER->CNT = 0;
    return request_irq(HR_TIMER_IRQ, stm32_timer_isr, NULL);
}

SYSINIT(stm32_hrtimer_init, SI_PREDRIVER_LEVEL, 10);
