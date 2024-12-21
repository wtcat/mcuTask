/*
 * Copyright 2024 wtcat
 */

#define HRTIMER_SOURCE_CODE
#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"
#include "basework/hrtimer_.h"

struct stm32_hrtimer {
    struct hrtimer_context base;
    uint64_t jiffies;
    uint32_t start;
};


/*
 * Timer parameter configure
 */
#define HR_TIMER         (TIM2)
#define HR_TIMER_IRQ     (TIM2_IRQn)
#define HR_TIMER_CLKEN() LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2)
#define HR_TIMER_MAX     (UINT32_MAX)

static __fastdata struct stm32_hrtimer stm32_hrtimer;

static __rte_always_inline uint32_t 
update_jiffies(struct stm32_hrtimer *ctx) {
    uint32_t now = HR_TIMER->CNT;
    uint32_t diff;
    if (now >= ctx->start)
        diff = now - ctx->start;
    else
        diff = now + (HR_TIMER_MAX - ctx->start);
    ctx->jiffies += diff;
    return now;
}

static int __fastcode 
load_next_event(struct stm32_hrtimer *ctx, uint32_t expire) {
    uint32_t now, next = HR_TIMER->CNT + expire;
    HR_TIMER->CCR1 = next;
    now = HR_TIMER->CNT;

    ctx->start = next - expire;
    if (now - next > expire)
        return -1;
    HR_TIMER->DIER |= TIM_DIER_CC1IE;
    return 0;
}

static int __fastcode 
load_next_timer(struct stm32_hrtimer *ctx, struct hrtimer *next_timer) {
_repeat:
    if (next_timer) {
        update_jiffies(ctx);
        if (rte_unlikely(next_timer->expire <= ctx->jiffies)) {
            TX_INTERRUPT_SAVE_AREA

            TX_DISABLE
            _hrtimer_expire((&ctx->base), next_timer, ctx->jiffies, 
                TX_RESTORE
                routine(next_timer);
                TX_DISABLE
            );
            TX_RESTORE
            goto _repeat;
        }

        uint64_t expire = next_timer->expire - ctx->jiffies;
        if (expire > (uint64_t)HR_TIMER_MAX)
            expire = HR_TIMER_MAX;

        return load_next_event(ctx, (uint32_t)expire);
    }
    return 0;
}

static void __fastcode 
stm32_timer_isr(void *arg) {
    TX_INTERRUPT_SAVE_AREA

    if (HR_TIMER->SR & TIM_SR_CC1IF) {
        HR_TIMER->SR &= ~TIM_SR_CC1IF;
        HR_TIMER->DIER &= ~TIM_DIER_CC1IE;

        struct stm32_hrtimer *ctx = arg;

        TX_DISABLE
        struct hrtimer *timer = _hrtimer_first(&ctx->base);
        update_jiffies(ctx);

        uint64_t now = ctx->jiffies;
        _hrtimer_expire((&ctx->base), timer, now, 
            TX_RESTORE
            routine(timer);
            TX_DISABLE
        );
        load_next_timer(ctx, timer);
        TX_RESTORE
    }
}

void __fastcode 
hrtimer_init(struct hrtimer *timer) {
    memset(timer, 0, sizeof(*timer));
    _hrtimer_set_state(timer, HRTIMER_INACTIVE);
}

int __fastcode 
hrtimer_start(struct hrtimer *timer, uint64_t expire) {
    struct stm32_hrtimer *ctx = &stm32_hrtimer;
    scoped_guard(os_irq) {
        update_jiffies(&stm32_hrtimer);
        if (_hrtimer_insert(&ctx->base, timer, ctx->jiffies + expire)) {
            if (expire > (uint64_t)HR_TIMER_MAX)
                expire = HR_TIMER_MAX;
            return load_next_event(ctx, expire);
        }
    }
    return 0;
}

int __fastcode 
hrtimer_stop(struct hrtimer *timer) {
    struct stm32_hrtimer *ctx = &stm32_hrtimer;
    scoped_guard(os_irq) {
        if (_hrtimer_remove(&ctx->base, timer))
            return load_next_timer(ctx, timer);
    }
    return 0;
}

int stm32_hrtimer_init(void) {
    int err;

    err = request_irq(HR_TIMER_IRQ, stm32_timer_isr, NULL);
    if (!err) {
        LL_TIM_InitTypeDef TIM_InitStruct;

        HR_TIMER_CLKEN();
        LL_TIM_StructInit(&TIM_InitStruct);
        TIM_InitStruct.Prescaler = HR_TIMER_PRESCALER - 1;
        LL_TIM_Init(HR_TIMER, &TIM_InitStruct);
        HR_TIMER->CR1 |= TIM_CR1_CEN | TIM_CR1_UDIS;
    }

    return err;
}

SYSINIT(stm32_hrtimer_init, SI_PREDRIVER_LEVEL, 10);
