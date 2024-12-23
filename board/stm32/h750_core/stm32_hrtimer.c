/*
 * Copyright 2024 wtcat
 */

#include "basework/compiler_types.h"
#define HRTIMER_SOURCE_CODE
#define TX_USE_BOARD_PRIVATE

#include <errno.h>
#include <inttypes.h>

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

#define HR_TIMER_MAX_ERROR (240 / HR_TIMER_PRESCALER / 10 + 1) /* 100ns */

static __fastdata struct stm32_hrtimer stm32_hrtimer;

#ifdef HRTIMER_DEBUG_ON
#include "subsys/cli/cli.h"

static void stm32_hrtimer_dump(void) {
    printk("Timer2 register dump:\n");
    printk("CR1    = 0x%08"PRIx32"\n", HR_TIMER->CR1);
    printk("CR2    = 0x%08"PRIx32"\n", HR_TIMER->CR2);
    printk("DIER   = 0x%08"PRIx32"\n", HR_TIMER->DIER);
    printk("SR     = 0x%08"PRIx32"\n", HR_TIMER->SR);
    printk("EGR    = 0x%08"PRIx32"\n", HR_TIMER->EGR);
    printk("CNT    = 0x%08"PRIx32"\n", HR_TIMER->CNT);
    printk("CCR1   = 0x%08"PRIx32"\n", HR_TIMER->CCR1);
    printk("PSC    = 0x%08"PRIx32"\n", HR_TIMER->PSC);
    printk("ARR    = 0x%08"PRIx32"\n", HR_TIMER->ARR);
    printk("RCR    = 0x%08"PRIx32"\n", HR_TIMER->RCR);
}

static int stm32_hrtimer_cmd(struct cli_process *cli, int argc, 
    char *argv[]) {
    (void) cli;
    (void) argc;
    (void) argv;
    stm32_hrtimer_dump();
	return 0;
}

CLI_CMD(timdump, "timdump",
    "SHow timer register status",
    stm32_hrtimer_cmd
);
#endif

static __rte_always_inline uint64_t 
current_jiffies(struct stm32_hrtimer *ctx) {
    if (rte_unlikely(HR_TIMER->SR & TIM_SR_UIF))
        return ctx->jiffies + HR_TIMER_MAX + HR_TIMER->CNT;
    return ctx->jiffies + HR_TIMER->CNT;
}

static __rte_always_inline int 
load_next_event(struct stm32_hrtimer *ctx, uint32_t expire, struct hrtimer *timer) {
    uint32_t now = HR_TIMER->CNT;
    uint32_t next = now + expire;

    HR_TIMER->CCR1 = next;
    ctx->start = now;

    now = HR_TIMER->CNT;
    if (next - now > expire) {
        printk("Time error\n");
        return -ETIME;
    }

    HR_TIMER->DIER |= TIM_DIER_CC1IE;
    return 0;
}

static __rte_always_inline int 
load_next_timer(struct stm32_hrtimer *ctx, struct hrtimer *next_timer) {
    if (next_timer) {
        uint64_t jiffies = current_jiffies(ctx);
        uint64_t expire;

        if (next_timer->expire >= jiffies) {
            expire = next_timer->expire - jiffies;
            if (expire < HR_TIMER_MAX_ERROR)
                expire = HR_TIMER_MAX_ERROR;
        } else {
            expire = HR_TIMER_MAX_ERROR;
        }
        return load_next_event(ctx, (uint32_t)expire, next_timer);
    }
    return 0;
}

static void __fastcode 
stm32_timer_isr(void *arg) {
    TX_INTERRUPT_SAVE_AREA
    struct stm32_hrtimer *ctx = arg;
    uint32_t sr = HR_TIMER->SR;

    HR_TIMER->SR = 0;
    if (rte_unlikely(sr & TIM_SR_UIF)) {
        ctx->jiffies += HR_TIMER_MAX;
        printk("Timer overflow\n");
    }

    if (sr & TIM_SR_CC1IF) {
        HR_TIMER->DIER &= ~TIM_DIER_CC1IE;
        
        TX_DISABLE
        struct hrtimer *timer = _hrtimer_first(&ctx->base);
        if (timer) {
            _hrtimer_expire((&ctx->base), timer, current_jiffies(ctx), 
                TX_RESTORE
                routine(timer);
                TX_DISABLE
            );
            load_next_timer(ctx, timer);
        }
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
        if (_hrtimer_insert(&ctx->base, timer, current_jiffies(ctx) + expire)) {
            if (rte_unlikely(expire > (uint64_t)HR_TIMER_MAX))
                expire = HR_TIMER_MAX;
            return load_next_event(ctx, expire, timer);
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

    err = request_irq(HR_TIMER_IRQ, stm32_timer_isr, &stm32_hrtimer);
    if (!err) {
        LL_TIM_InitTypeDef TIM_InitStruct;

        HR_TIMER_CLKEN();
        LL_TIM_StructInit(&TIM_InitStruct);
        TIM_InitStruct.Prescaler = HR_TIMER_PRESCALER - 1;
        LL_TIM_Init(HR_TIMER, &TIM_InitStruct);
        LL_TIM_GenerateEvent_UPDATE(HR_TIMER);
        HR_TIMER->SR = 0;
        HR_TIMER->DIER = TIM_DIER_UIE;
        HR_TIMER->CR1 |= TIM_CR1_CEN | TIM_CR1_UDIS;
    }

    return err;
}

SYSINIT(stm32_hrtimer_init, SI_PREDRIVER_LEVEL, 10);
