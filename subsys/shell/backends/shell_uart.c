/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <base/sys/atomic.h>
#include <base/sys/ring_buffer.h>
#include <base/log.h>

#include <subsys/shell/shell.h>
#include <subsys/shell/shell_uart.h>
#include <subsys/shell/shell_ops.h>

#include <drivers/uart.h>
#include <drivers/uart_async_rx.h>

#ifndef LOG_WRN
#define LOG_WRN(...)
#endif
#ifndef CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE 0
#endif
#ifndef CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT
#define CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT 0
#endif

#ifdef CONFIG_SHELL_BACKEND_SERIAL_RX_POLL_PERIOD
#define RX_POLL_PERIOD TX_MSEC(CONFIG_SHELL_BACKEND_SERIAL_RX_POLL_PERIOD)
#else
#define RX_POLL_PERIOD TX_NO_WAIT
#endif

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
NET_BUF_POOL_DEFINE(smp_shell_rx_pool, CONFIG_MCUMGR_TRANSPORT_SHELL_RX_BUF_COUNT,
					SMP_SHELL_RX_BUF_SIZE, 0, NULL);
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */

static void async_callback(const struct device *dev, struct uart_event *evt,
						   void *user_data) {
	struct shell_uart_async *sh_uart = (struct shell_uart_async *)user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		tx_semaphore_ceiling_put(&sh_uart->tx_sem, 1);
		break;
	case UART_RX_RDY:
		uart_async_rx_on_rdy(&sh_uart->async_rx, evt->data.rx.buf, evt->data.rx.len);
		sh_uart->common.handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_uart->common.context);
		break;
	case UART_RX_BUF_REQUEST: {
		uint8_t *buf = uart_async_rx_buf_req(&sh_uart->async_rx);
		size_t len = uart_async_rx_get_buf_len(&sh_uart->async_rx);

		if (buf) {
			int err = uart_rx_buf_rsp(dev, buf, len);

			if (err < 0) {
				uart_async_rx_on_buf_rel(&sh_uart->async_rx, buf);
			}
		} else {
			atomic_inc(&sh_uart->pending_rx_req);
		}

		break;
	}
	case UART_RX_BUF_RELEASED:
		uart_async_rx_on_buf_rel(&sh_uart->async_rx, evt->data.rx_buf.buf);
		break;
	case UART_RX_DISABLED:
		break;
	default:
		break;
	};
}

static void uart_rx_handle(const struct device *dev,
						   struct shell_uart_int_driven *sh_uart) {
	uint8_t *data;
	uint32_t len;
	uint32_t rd_len;
	bool new_data = false;
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	struct smp_shell_data *const smp = &sh_uart->common.smp;
#endif

	do {
		len = ring_buf_put_claim(&sh_uart->rx_ringbuf, &data, sh_uart->rx_ringbuf.size);

		if (len > 0) {
			rd_len = uart_fifo_read(dev, data, len);

			/* If there is any new data to be either taken into
			 * ring buffer or consumed by the SMP, signal the
			 * shell_thread.
			 */
			if (rd_len > 0) {
				new_data = true;
			}
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
			/* Divert bytes from shell handling if it is
			 * part of an mcumgr frame.
			 */
			size_t i = smp_shell_rx_bytes(smp, data, rd_len);

			rd_len -= i;

			if (rd_len) {
				for (uint32_t j = 0; j < rd_len; j++) {
					data[j] = data[i + j];
				}
			}
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */
			int err = ring_buf_put_finish(&sh_uart->rx_ringbuf, rd_len);
			(void)err;
			rte_assert(err == 0);
		} else {
			uint8_t dummy;

			/* No space in the ring buffer - consume byte. */
			LOG_WRN("RX ring buffer full.");

			rd_len = uart_fifo_read(dev, &dummy, 1);
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
			/* If successful in getting byte from the fifo, try
			 * feeding it to SMP as a part of mcumgr frame.
			 */
			if ((rd_len != 0) && (smp_shell_rx_bytes(smp, &dummy, 1) == 1)) {
				new_data = true;
			}
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */
		}
	} while (rd_len && (rd_len == len));

	if (new_data) {
		sh_uart->common.handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_uart->common.context);
	}
}

static bool uart_dtr_check(const struct device *dev) {
	BUILD_ASSERT(!IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_CHECK_DTR) ||
					 IS_ENABLED(CONFIG_UART_LINE_CTRL),
				 "DTR check requires CONFIG_UART_LINE_CTRL");

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_CHECK_DTR)) {
		int dtr, err;

		err = uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, (uint32_t *)&dtr);
		if (err == -ENOSYS || err == -ENOTSUP) {
			return true;
		}

		return dtr;
	}

	return true;
}

static void dtr_timer_handler(ULONG timer) {
	struct shell_uart_int_driven *sh_uart = (struct shell_uart_int_driven *)timer;

	if (!uart_dtr_check(sh_uart->common.dev)) {
		return;
	}

	/* DTR is active, stop timer and start TX */
	tx_timer_deactivate(&sh_uart->dtr_timer);
	uart_irq_tx_enable(sh_uart->common.dev);
}

static void uart_tx_handle(const struct device *dev,
						   struct shell_uart_int_driven *sh_uart) {
	uint32_t len;
	const uint8_t *data;

	if (!uart_dtr_check(dev)) {
		/* Wait for DTR signal before sending anything to output. */
		uart_irq_tx_disable(dev);
		tx_timer_activate(&sh_uart->dtr_timer);
		return;
	}

	len = ring_buf_get_claim(&sh_uart->tx_ringbuf, (uint8_t **)&data,
							 sh_uart->tx_ringbuf.size);
	if (len) {
		int err;

		len = uart_fifo_fill(dev, data, len);
		err = ring_buf_get_finish(&sh_uart->tx_ringbuf, len);
		rte_assert(err == 0);
		ARG_UNUSED(err);
	} else {
		uart_irq_tx_disable(dev);
		sh_uart->tx_busy = 0;
	}

	sh_uart->common.handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_uart->common.context);
}

static void uart_callback(const struct device *dev, void *user_data) {
	struct shell_uart_int_driven *sh_uart = (struct shell_uart_int_driven *)user_data;

	uart_irq_update(dev);

	if (uart_irq_rx_ready(dev)) {
		uart_rx_handle(dev, sh_uart);
	}

	if (uart_irq_tx_ready(dev)) {
		uart_tx_handle(dev, sh_uart);
	}
}

static void irq_init(struct shell_uart_int_driven *sh_uart) {
	const struct device *dev = sh_uart->common.dev;

	ring_buf_init(&sh_uart->rx_ringbuf, CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE,
				  sh_uart->rx_buf);
	ring_buf_init(&sh_uart->tx_ringbuf, CONFIG_SHELL_BACKEND_SERIAL_TX_RING_BUFFER_SIZE,
				  sh_uart->tx_buf);
	sh_uart->tx_busy = 0;
	uart_irq_callback_user_data_set(dev, uart_callback, (void *)sh_uart);
	uart_irq_rx_enable(dev);

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_CHECK_DTR)) {
		tx_timer_create(&sh_uart->dtr_timer, "shell_dtr", dtr_timer_handler,
						(ULONG)sh_uart, TX_MSEC(100), TX_MSEC(100), TX_NO_ACTIVATE);
	}
}

static int rx_enable(const struct device *dev, uint8_t *buf, size_t len) {
	return uart_rx_enable(dev, buf, len, 10000);
}

static void async_init(struct shell_uart_async *sh_uart) {
	const struct device *dev = sh_uart->common.dev;
	struct uart_async_rx *async_rx = &sh_uart->async_rx;
	int err;

	sh_uart->async_rx_config = (struct uart_async_rx_config){
		.buffer = sh_uart->rx_data,
		.length = ASYNC_RX_BUF_SIZE,
		.buf_cnt = CONFIG_SHELL_BACKEND_SERIAL_ASYNC_RX_BUFFER_COUNT,
	};

	tx_semaphore_create(&sh_uart->tx_sem, (CHAR *)"shell_async_uart", 0);
	// k_sem_init(&sh_uart->tx_sem, 0, 1);

	err = uart_async_rx_init(async_rx, &sh_uart->async_rx_config);
	(void)err;
	rte_assert(err == 0);

	uint8_t *buf = uart_async_rx_buf_req(async_rx);

	err = uart_callback_set(dev, async_callback, (void *)sh_uart);
	(void)err;
	rte_assert(err == 0);

	err = rx_enable(dev, buf, uart_async_rx_get_buf_len(async_rx));
	(void)err;
	rte_assert(err == 0);
}

static void polling_rx_timeout_handler(ULONG timer) {
	uint8_t c;
	struct shell_uart_polling *sh_uart = (struct shell_uart_polling *)timer;

	while (uart_poll_in(sh_uart->common.dev, &c) == 0) {
		if (ring_buf_put(&sh_uart->rx_ringbuf, &c, 1) == 0U) {
			/* ring buffer full. */
			LOG_WRN("RX ring buffer full.");
		}
		sh_uart->common.handler(SHELL_TRANSPORT_EVT_RX_RDY, sh_uart->common.context);
	}
}

static void polling_init(struct shell_uart_polling *sh_uart) {
	tx_timer_create(&sh_uart->rx_timer, "shell_rx", polling_rx_timeout_handler,
					(ULONG)sh_uart, RX_POLL_PERIOD, RX_POLL_PERIOD, TX_NO_ACTIVATE);
	ring_buf_init(&sh_uart->rx_ringbuf, CONFIG_SHELL_BACKEND_SERIAL_RX_RING_BUFFER_SIZE,
				  sh_uart->rx_buf);
}

static int init(const struct shell_transport *transport, const void *config,
				shell_transport_handler_t evt_handler, void *context) {
	struct shell_uart_common *common = (struct shell_uart_common *)transport->ctx;

	common->dev = (const struct device *)config;
	common->handler = evt_handler;
	common->context = context;

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	common->smp.buf_pool = &smp_shell_rx_pool;
	k_fifo_init(&common->smp.buf_ready);
#endif

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_ASYNC)) {
		async_init((struct shell_uart_async *)transport->ctx);
	} else if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN)) {
		irq_init((struct shell_uart_int_driven *)transport->ctx);
	} else {
		polling_init((struct shell_uart_polling *)transport->ctx);
	}

	return 0;
}

static void irq_uninit(struct shell_uart_int_driven *sh_uart) {
	const struct device *dev = sh_uart->common.dev;

	tx_timer_delete(&sh_uart->dtr_timer);
	uart_irq_tx_disable(dev);
	uart_irq_rx_disable(dev);
}

static void async_uninit(struct shell_uart_async *sh_uart) {}

static void polling_uninit(struct shell_uart_polling *sh_uart) {
	tx_timer_delete(&sh_uart->rx_timer);
}

static int uninit(const struct shell_transport *transport) {
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_ASYNC)) {
		async_uninit((struct shell_uart_async *)transport->ctx);
	} else if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN)) {
		irq_uninit((struct shell_uart_int_driven *)transport->ctx);
	} else {
		polling_uninit((struct shell_uart_polling *)transport->ctx);
	}

	return 0;
}

static int enable(const struct shell_transport *transport, bool blocking_tx) {
	struct shell_uart_common *sh_uart = (struct shell_uart_common *)transport->ctx;

	sh_uart->blocking_tx =
		blocking_tx || IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_FORCE_TX_BLOCKING_MODE);

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN) && blocking_tx) {
		uart_irq_tx_disable(sh_uart->dev);
	}

	return 0;
}

static int polling_write(struct shell_uart_common *sh_uart, const void *data,
						 size_t length, size_t *cnt) {
	const uint8_t *data8 = (const uint8_t *)data;

	for (size_t i = 0; i < length; i++) {
		uart_poll_out(sh_uart->dev, data8[i]);
	}

	*cnt = length;

	sh_uart->handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_uart->context);

	return 0;
}

static int irq_write(struct shell_uart_int_driven *sh_uart, const void *data,
					 size_t length, size_t *cnt) {
	*cnt = ring_buf_put(&sh_uart->tx_ringbuf, data, length);

	if (atomic_set(&sh_uart->tx_busy, 1) == 0) {
		uart_irq_tx_enable(sh_uart->common.dev);
	}

	return 0;
}

static int async_write(struct shell_uart_async *sh_uart, const void *data, size_t length,
					   size_t *cnt) {
	int err;

	err = uart_tx(sh_uart->common.dev, data, length, TX_WAIT_FOREVER);
	if (err < 0) {
		*cnt = 0;
		return err;
	}

	err = tx_semaphore_get(&sh_uart->tx_sem, TX_WAIT_FOREVER);
	*cnt = length;

	sh_uart->common.handler(SHELL_TRANSPORT_EVT_TX_RDY, sh_uart->common.context);

	return err;
}

static int write_uart(const struct shell_transport *transport, const void *data,
					  size_t length, size_t *cnt) {
	struct shell_uart_common *sh_uart = (struct shell_uart_common *)transport->ctx;

	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN)) {
		return irq_write((struct shell_uart_int_driven *)transport->ctx, data, length,
						 cnt);
	} else if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_POLLING) || sh_uart->blocking_tx) {
		return polling_write(sh_uart, data, length, cnt);

	} else {
		return async_write((struct shell_uart_async *)transport->ctx, data, length, cnt);
	}
}

static int irq_read(struct shell_uart_int_driven *sh_uart, void *data, size_t length,
					size_t *cnt) {
	*cnt = ring_buf_get(&sh_uart->rx_ringbuf, data, length);

	return 0;
}

static int polling_read(struct shell_uart_polling *sh_uart, void *data, size_t length,
						size_t *cnt) {
	*cnt = ring_buf_get(&sh_uart->rx_ringbuf, data, length);

	return 0;
}

static int async_read(struct shell_uart_async *sh_uart, void *data, size_t length,
					  size_t *cnt) {
	uint8_t *buf;
	size_t blen;
	struct uart_async_rx *async_rx = &sh_uart->async_rx;

	blen = uart_async_rx_data_claim(async_rx, &buf, length);
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	struct smp_shell_data *const smp = &sh_uart->common.smp;
	size_t sh_cnt = 0;

	for (size_t i = 0; i < blen; i++) {
		if (smp_shell_rx_bytes(smp, &buf[i], 1) == 0) {
			((uint8_t *)data)[sh_cnt++] = buf[i];
		}
	}
#else
	size_t sh_cnt = blen;

	memcpy(data, buf, blen);
#endif
	bool buf_available = uart_async_rx_data_consume(async_rx, sh_cnt);
	*cnt = sh_cnt;

	if (sh_uart->pending_rx_req && buf_available) {
		uint8_t *buf = uart_async_rx_buf_req(async_rx);
		size_t len = uart_async_rx_get_buf_len(async_rx);
		int err;

		rte_assert(buf != NULL);
		atomic_dec(&sh_uart->pending_rx_req);
		err = uart_rx_buf_rsp(sh_uart->common.dev, buf, len);
		/* If it is too late and RX is disabled then re-enable it. */
		if (err < 0) {
			if (err == -EACCES) {
				sh_uart->pending_rx_req = 0;
				err = rx_enable(sh_uart->common.dev, buf, len);
			} else {
				return err;
			}
		}
	}

	return 0;
}

static int read_uart(const struct shell_transport *transport, void *data, size_t length,
					 size_t *cnt) {
	if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_INTERRUPT_DRIVEN)) {
		return irq_read((struct shell_uart_int_driven *)transport->ctx, data, length,
						cnt);
	} else if (IS_ENABLED(CONFIG_SHELL_BACKEND_SERIAL_API_ASYNC)) {
		return async_read((struct shell_uart_async *)transport->ctx, data, length, cnt);
	} else {
		return polling_read((struct shell_uart_polling *)transport->ctx, data, length,
							cnt);
	}
}

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
static void update(const struct shell_transport *transport) {
	/*
	 * This is dependent on the fact that `struct shell_uart_common`
	 * is always the first member, regardless of the UART configuration
	 */
	struct shell_uart_common *sh_uart = (struct shell_uart_common *)transport->ctx;

	smp_shell_process(&sh_uart->smp);
}
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */

const struct shell_transport_api shell_uart_transport_api = {
	.init = init,
	.uninit = uninit,
	.enable = enable,
	.write = write_uart,
	.read = read_uart,
#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
	.update = update,
#endif /* CONFIG_MCUMGR_TRANSPORT_SHELL */
};

SHELL_UART_DEFINE(shell_transport_uart);
SHELL_DEFINE(shell_uart, CONFIG_SHELL_PROMPT_UART, &shell_transport_uart,
			 CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_SIZE,
			 CONFIG_SHELL_BACKEND_SERIAL_LOG_MESSAGE_QUEUE_TIMEOUT, SHELL_FLAG_OLF_CRLF);

#ifdef CONFIG_MCUMGR_TRANSPORT_SHELL
struct smp_shell_data *shell_uart_smp_shell_data_get_ptr(void) {
	struct shell_uart_common *common =
		(struct shell_uart_common *)shell_transport_uart.ctx;

	return &common->smp;
}
#endif

static int shell_uart_format(void *context, const char *fmt, va_list ap) {
	shell_vfprintf(&shell_uart, SHELL_NORMAL, fmt, ap);
	return 0;
}

static int enable_shell_uart(void) {
	const struct device *const dev = device_find(CONFIG_SHELL_DEVICE);
	bool log_backend = CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL > 0;
	uint32_t level = CONFIG_SHELL_BACKEND_SERIAL_LOG_LEVEL;

	static const struct shell_backend_config_flags cfg_flags =
		SHELL_DEFAULT_BACKEND_CONFIG_FLAGS;

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_SHELL)) {
		// smp_shell_init();
	}

	rte_assert(dev != NULL);
	shell_init(&shell_uart, dev, cfg_flags, log_backend, level);
	pr_log_redirect(shell_uart_format, NULL);

	return 0;
}

SYSINIT(enable_shell_uart, SI_APPLICATION_LEVEL, 50);

const struct shell *shell_backend_uart_get_ptr(void) {
	return &shell_uart;
}
