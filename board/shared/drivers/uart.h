/*
 * Copyright 2024 wtcat
 */

#ifndef DRIVERS_UART_H_
#define DRIVERS_UART_H_

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C"{
#endif

struct device;

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

/*
 * Device control command
 */
#define UART_CONFIGURE  1
#define UART_SETSPEED   2

int uart_open(const char *name, struct device **pdev);
int uart_close(struct device *dev);
int uart_control(struct device *dev, unsigned int cmd, void *arg);
ssize_t uart_write(struct device *dev, const char *buf, size_t len, 
    unsigned int options);
ssize_t uart_read(struct device *dev, char *buf, size_t len, 
    unsigned int options);

#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_UART_H_ */
