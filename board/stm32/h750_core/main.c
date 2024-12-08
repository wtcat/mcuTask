/*
 * Copyright 2024 wtcat
 */
#define TX_USE_BOARD_PRIVATE

#include "tx_api.h"
#include "basework/rte_cpu.h"
#include "basework/os/osapi.h"
#include "subsys/cli/cli.h"


static void demo_test(void);
void tx_application_define(void *unused) {
	/* Initialize HAL layer */
	HAL_Init();

	/* Clock initialize */
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
	LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);

    do_sysinit();
    demo_test();
}

static void gpio_key_isr(int line, void *arg) {
    (void) arg;
    printk("gpio_key_isr: line(%d)\n", line);
}

static void __rte_unused demo_thread_2(ULONG arg) {
    int count = 0;

    for ( ; ; ) {
        printk("Thread-2: count(%d)\n", count++);
        tx_thread_sleep(TX_MSEC(1000));
    }
}

static void demo_test(void) {
    static TX_THREAD tx_demo2;
    static ULONG txdemo_stack2[1024/sizeof(ULONG)] __rte_section(".dtcm");
    tx_thread_create(&tx_demo2, "demo", demo_thread_2, 0, txdemo_stack2, 
        sizeof(txdemo_stack2), 11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);

    static ULONG stack[256];
    cli_run("uart1", 15, stack, sizeof(stack), "[task]# ", &_cli_ifdev_uart);

    gpio_request_irq(GPIO_USER_KEY1, gpio_key_isr, NULL, false, true);
					 
}
