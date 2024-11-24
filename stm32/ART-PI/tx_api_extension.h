/*
 * Copyright 2024 wtcat
 */

#ifndef TX_API_EXTENSION_H_
#define TX_API_EXTENSION_H_

#include <sys/types.h>
#include "basework/generic.h"
#include "basework/linker.h"

#ifdef __cplusplus
extern "C"{
#endif

#if defined(__GNUC__) || defined(__clang__)
#include "basework/cleanup.h"

typedef struct TX_MUTEX_STRUCT TX_MUTEX;
DEFINE_GUARD(os_mutex, TX_MUTEX *, tx_mutex_get(_T, 0xFFFFFFFFUL), tx_mutex_put(_T))

DEFINE_LOCK_GUARD_0(os_irq, (_T)->key = __disable_interrupts(), __restore_interrupt((_T)->key), unsigned int key)	
#endif /* __GNUC__ ||  __clang__ */

/*
 * Device I/O options
 */
#define O_NOBLOCK 0x0001

/*
 * Custom define section
 */
#define __fastcode  __rte_section(".itcm")
#define __fastbss   __rte_section(".fastbss")
#define __fastdata  __rte_section(".fastdata")

/*
 * System intitialize 
 */
struct sysinit_item {
   int (*handler)(void);
   const char *name;
};

/* system initialize order */
#define SI_PREDRIVER_ORDER     40
#define SI_DRIVER_ORDER        50
#define SI_APPLICATION_ORDER   150

#define SYSINIT(_handler, _order) \
    enum { __enum_##_handler = _order}; \
    __SYSINIT(_handler, _order)

#define __SYSINIT(_handler, _order) \
   static LINKER_ROSET_ITEM_ORDERED(sysinit, struct sysinit_item, entry, _order) = { \
      .handler = _handler, \
      .name = #_handler \
   }

/*
 * Memory allocate interface
 */
#define GMF_KERNEL 0x0001
#define GMF_WAIT   0x0002

void *__kmalloc(size_t size, unsigned int gmf);
void *__kzalloc(size_t size, unsigned int gmf);
void  __kfree(void *ptr);

#define kmalloc(s, f) __kmalloc(s, f)
#define kzalloc(s, f) __kzalloc(s, f)
#define kfree(p)      __kfree(p)

/*
 * Platform interface
 */
int printk(const char *fmt, ...) __rte_printf(1, 2);
int request_irq(int irq, void (*handler)(void *), void *arg);
int remove_irq(int irq, void (*handler)(void *), void *arg);

/*
 * uart driver
 */
enum uart_datawidth {
    kUartDataWidth_7B,
    kUartDataWidth_8B,
    kUartDataWidth_9B,
};

enum uart_stopwidth {
    kUartStopWidth_1B,
    kUartStopWidth_2B,
};

enum uart_parity {
    kUartParityNone,
    kUartParityOdd,
    kUartParityEven
};

struct uart_param {
    unsigned int baudrate;
    enum uart_datawidth nb_data;
    enum uart_stopwidth nb_stop;
    enum uart_parity parity;
    bool hwctrl;
};

#define UART_SET_FORMAT 1
#define UART_SET_SPEED  2

int uart_open(const char *name, void **pdev);
int uart_close(void *dev);
int uart_control(void *dev, unsigned int cmd, void *arg);
ssize_t uart_write(void *dev, const char *buf, size_t len, unsigned int options);
ssize_t uart_read(void *dev, char *buf, size_t len, unsigned int options);
void console_putc(char c);


#ifdef __cplusplus
}
#endif
#endif /* TX_API_EXTENSION_H_ */
