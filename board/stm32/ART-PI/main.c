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
    do_sysinit();
    demo_test();
}

int main(void) {
    tx_kernel_enter();
    return 0;
}

static void gpio_key_isr(int line, void *arg) {
    (void) arg;

    printk("gpio_key_isr: line(%d)\n", line);
}

static char dma_buffer[128] __rte_aligned(32);
static void demo_thread_1(ULONG arg) {
    // int count = 0;
    void *dev = NULL;

    gpio_request_irq(GPIO_USER_KEY1, gpio_key_isr, NULL, false, true);

    if (uart_open("uart1", &dev) < 0)
        return;

    static const char text[] = {"hello, USART1 (Copyright 2024 wtcat)\n"};
    uart_write(dev, text, sizeof(text) - 1, 0);
    printk("demo_thread_1 write okay\n");
    for ( ; ; ) {
        memset(dma_buffer, 0, sizeof(dma_buffer));
        size_t bytes = uart_read(dev, dma_buffer, sizeof(dma_buffer) - 1, 0);
        if (bytes > 0) {
            printk("recv: bytes(%d)\n", bytes);
            dma_buffer[bytes] = '\0';
            uart_write(dev, dma_buffer, bytes, 0);
        }
    }
}

static void __rte_unused demo_thread_2(ULONG arg) {
    int count = 0;

    for ( ; ; ) {
        printk("Thread-2: count(%d)\n", count++);
        tx_thread_sleep(TX_MSEC(1000));
    }
}

static void demo_test(void) {
    static TX_THREAD tx_demo1;
    static ULONG txdemo_stack1[1024/sizeof(ULONG)] __rte_section(".dtcm");

    // static TX_THREAD tx_demo2;
    // static ULONG txdemo_stack2[1024/sizeof(ULONG)] __rte_section(".dtcm");

    tx_thread_create(&tx_demo1, "demo", demo_thread_1, 0, txdemo_stack1, 
        sizeof(txdemo_stack1), 10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
    // tx_thread_create(&tx_demo2, "demo", demo_thread_2, 0, txdemo_stack2, 
    //     sizeof(txdemo_stack2), 11, 11, TX_NO_TIME_SLICE, TX_AUTO_START);
    // cli_run("uart1", 15);
}


static int test_cli(int argc, char *argv[]) {
    printk("test-cli\n");
    return 0;
}

CLI_CMD(test, "test command line interface", test_cli)
