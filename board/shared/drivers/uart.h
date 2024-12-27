/*
 * Copyright 2024 wtcat
 */

#ifndef DRIVERS_UART_H_
#define DRIVERS_UART_H_

#include <stdbool.h>
#include <sys/types.h>

#include "drivers/device.h"

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

DEVICE_CLASS_DEFINE(uart_device,
    int (*configure)(struct device *dev, struct uart_param *up);
    ssize_t (*write)(struct device *dev, const char *buf, size_t len, 
        unsigned int options);
    ssize_t (*read)(struct device *dev, char *buf, size_t len, 
        unsigned int options);
);

/*
 * Device control command
 */
#define UART_SETSPEED   2

#ifdef TX_UART_DEVICE_STUB
int uart_configure(struct device *dev, struct uart_param *up);
ssize_t uart_write(struct device *dev, const char *buf, size_t len, 
    unsigned int options);
ssize_t uart_read(struct device *dev, char *buf, size_t len, 
    unsigned int options);

#else /* */
static inline int
uart_configure(struct device *dev, struct uart_param *up) {
    struct uart_device *udev = (struct uart_device *)dev;
    if (udev->control)
        return udev->configure(dev, up);
    return -ENOSYS;
}

static inline ssize_t 
uart_write(struct device *dev, const char *buf, size_t len, 
    unsigned int options) {
    struct uart_device *udev = (struct uart_device *)dev;
    return udev->write(dev, buf, len, options);
}

static inline ssize_t 
uart_read(struct device *dev, char *buf, size_t len, 
    unsigned int options) {
    struct uart_device *udev = (struct uart_device *)dev;
    return udev->read(dev, buf, len, options);
}
#endif /* */

#ifdef __cplusplus
}
#endif
#endif /* DRIVERS_UART_H_ */
