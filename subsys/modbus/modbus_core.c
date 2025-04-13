/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define pr_fmt(fmt) "[modbus_core]: " fmt "\n"

#include <string.h>
#include <tx_api.h>

#include <base/log.h>
#include <base/linker.h>
#include <base/sys/byteorder.h>
#include <subsys/modbus/modbus_internal.h>


LINKER_RWSET(modbus, struct modbus_context);

static TX_THREAD modbus_thread;
static ULONG modbus_stack[CONFIG_MODBUS_THREAD_STACK_SIZE / sizeof(ULONG)];
static TX_QUEUE modbus_queue;
static void *modbus_queue_buffer[CONFIG_MODBUS_QUEUE_SIZE];

static struct modbus_context *const mb_ctx_tbl = (void *)LINKER_SET_BEGIN(modbus);
static uint8_t mb_ctx_size;


static void modbus_rx_server(void *param) {
	struct modbus_context *ctx;
	(void) param;

	for (;;) {
		int err = tx_queue_receive(&modbus_queue, &ctx, TX_WAIT_FOREVER);
		if (err) {
			pr_err("Failed(%d) read modbus message queue\n", err);
			continue;
		}

		switch (ctx->mode) {
		case MODBUS_MODE_RTU:
		case MODBUS_MODE_ASCII:
			if (IS_ENABLED(CONFIG_MODBUS_SERIAL)) {
				modbus_serial_rx_disable(ctx);
				ctx->rx_adu_err = modbus_serial_rx_adu(ctx);
			}
			break;
		case MODBUS_MODE_RAW:
			if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU)) {
				ctx->rx_adu_err = modbus_raw_rx_adu(ctx);
			}
			break;
		default:
			pr_err("Unknown MODBUS mode");
			continue;
		}

		if (ctx->client == true) {
			tx_semaphore_ceiling_put(&ctx->client_wait_sem, 1);
		} else if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
			bool respond = modbus_server_handler(ctx);

			if (respond) {
				modbus_tx_adu(ctx);
			} else {
				pr_dbg("Server has dropped frame");
			}

			switch (ctx->mode) {
			case MODBUS_MODE_RTU:
			case MODBUS_MODE_ASCII:
				if (IS_ENABLED(CONFIG_MODBUS_SERIAL) && respond == false) {
					modbus_serial_rx_enable(ctx);
				}
				break;
			default:
				break;
			}
		}
	}
}

void modbus_tx_adu(struct modbus_context *ctx) {
	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) && modbus_serial_tx_adu(ctx)) {
			pr_err("Unsupported MODBUS serial mode");
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) && modbus_raw_tx_adu(ctx)) {
			pr_err("Unsupported MODBUS raw mode");
		}
		break;
	default:
		pr_err("Unknown MODBUS mode");
	}
}

int modbus_tx_wait_rx_adu(struct modbus_context *ctx) {
	tx_semaphore_get(&ctx->client_wait_sem, TX_NO_WAIT);
	modbus_tx_adu(ctx);

	if (tx_semaphore_get(&ctx->client_wait_sem, TX_USEC(ctx->rxwait_to))) {
		pr_warn("Client wait-for-RX timeout");
		return -ETIMEDOUT;
	}

	return ctx->rx_adu_err;
}

struct modbus_context *modbus_get_context(const uint8_t iface) {
	struct modbus_context *ctx;

	if (iface >= mb_ctx_size) {
		pr_err("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (!atomic_test_bit(&ctx->state, MODBUS_STATE_CONFIGURED)) {
		pr_err("Interface not configured");
		return NULL;
	}

	return ctx;
}

int modbus_iface_get_by_ctx(const struct modbus_context *ctx) {
	for (int i = 0; i < mb_ctx_size; i++) {
		if (&mb_ctx_tbl[i] == ctx) 
			return i;
	}
	return -ENODEV;
}

int modbus_iface_get_by_name(const char *iface_name) {
	for (int i = 0; i < mb_ctx_size; i++) {
		if (strcmp(iface_name, mb_ctx_tbl[i].iface_name) == 0)
			return i;
	}
	return -ENODEV;
}

static struct modbus_context *modbus_init_iface(const uint8_t iface) {
	struct modbus_context *ctx;

	if (!mb_ctx_size)
		mb_ctx_size = (LINKER_SET_END(modbus) - LINKER_SET_BEGIN(modbus));

	if (iface >= mb_ctx_size) {
		pr_err("Interface %u not available", iface);
		return NULL;
	}

	ctx = &mb_ctx_tbl[iface];

	if (atomic_test_and_set_bit(&ctx->state, MODBUS_STATE_CONFIGURED)) {
		pr_err("Interface already used");
		return NULL;
	}

	tx_mutex_create(&ctx->iface_lock, (CHAR *)"modbus", TX_INHERIT);
	tx_semaphore_create(&ctx->client_wait_sem, (CHAR *)"modbus", 0);
	ctx->server_work = &modbus_queue;

	return ctx;
}

static int modbus_user_fc_init(struct modbus_context *ctx,
							   struct modbus_iface_param param) {
	STAILQ_INIT(&ctx->user_defined_cbs);
	pr_dbg("Initializing user-defined function code support.");

	return 0;
}

int modbus_init_server(const int iface, struct modbus_iface_param param) {
	struct modbus_context *ctx = NULL;
	int rc = 0;

	if (!IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		pr_err("Modbus server support is not enabled");
		rc = -ENOTSUP;
		goto init_server_error;
	}

	if (param.server.user_cb == NULL) {
		pr_err("User callbacks should be available");
		rc = -EINVAL;
		goto init_server_error;
	}

	ctx = modbus_init_iface(iface);
	if (ctx == NULL) {
		rc = -EINVAL;
		goto init_server_error;
	}

	ctx->client = false;

	if (modbus_user_fc_init(ctx, param) != 0) {
		pr_err("Failed to init MODBUS user defined function codes");
		rc = -EINVAL;
		goto init_server_error;
	}

	switch (param.mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) && modbus_serial_init(ctx, param) != 0) {
			pr_err("Failed to init MODBUS over serial line");
			rc = -EINVAL;
			goto init_server_error;
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) && modbus_raw_init(ctx, param) != 0) {
			pr_err("Failed to init MODBUS raw ADU support");
			rc = -EINVAL;
			goto init_server_error;
		}
		break;
	default:
		pr_err("Unknown MODBUS mode");
		rc = -ENOTSUP;
		goto init_server_error;
	}

	ctx->unit_id = param.server.unit_id;
	ctx->mbs_user_cb = param.server.user_cb;
	if (IS_ENABLED(CONFIG_MODBUS_FC08_DIAGNOSTIC)) {
		modbus_reset_stats(ctx);
	}

	pr_dbg("Modbus interface %s initialized", ctx->iface_name);

	return 0;

init_server_error:
	if (ctx != NULL) {
		atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);
	}

	return rc;
}

int modbus_register_user_fc(const int iface, struct modbus_custom_fc *custom_fc) {
	struct modbus_context *ctx = modbus_get_context(iface);

	if (!custom_fc) {
		pr_err("Provided function code handler was NULL");
		return -EINVAL;
	}

	if (custom_fc->fc & BIT(7)) {
		pr_err("Function codes must have MSB of 0");
		return -EINVAL;
	}

	custom_fc->excep_code = MODBUS_EXC_NONE;

	pr_dbg("Registered new custom function code %d", custom_fc->fc);
	STAILQ_INSERT_TAIL(&ctx->user_defined_cbs, custom_fc, node);

	return 0;
}

int modbus_init_client(const int iface, struct modbus_iface_param param) {
	struct modbus_context *ctx = NULL;
	int rc = 0;

	if (!IS_ENABLED(CONFIG_MODBUS_CLIENT)) {
		pr_err("Modbus client support is not enabled");
		rc = -ENOTSUP;
		goto init_client_error;
	}

	ctx = modbus_init_iface(iface);
	if (ctx == NULL) {
		rc = -EINVAL;
		goto init_client_error;
	}

	ctx->client = true;

	switch (param.mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL) && modbus_serial_init(ctx, param) != 0) {
			pr_err("Failed to init MODBUS over serial line");
			rc = -EINVAL;
			goto init_client_error;
		}
		break;
	case MODBUS_MODE_RAW:
		if (IS_ENABLED(CONFIG_MODBUS_RAW_ADU) && modbus_raw_init(ctx, param) != 0) {
			pr_err("Failed to init MODBUS raw ADU support");
			rc = -EINVAL;
			goto init_client_error;
		}
		break;
	default:
		pr_err("Unknown MODBUS mode");
		rc = -ENOTSUP;
		goto init_client_error;
	}

	ctx->unit_id = 0;
	ctx->mbs_user_cb = NULL;
	ctx->rxwait_to = param.rx_timeout;

	return 0;

init_client_error:
	if (ctx != NULL) {
		atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);
	}

	return rc;
}

int modbus_disable(const uint8_t iface) {
	struct modbus_context *ctx;

	ctx = modbus_get_context(iface);
	if (ctx == NULL) {
		pr_err("Interface %u not initialized", iface);
		return -EINVAL;
	}

	switch (ctx->mode) {
	case MODBUS_MODE_RTU:
	case MODBUS_MODE_ASCII:
		if (IS_ENABLED(CONFIG_MODBUS_SERIAL)) {
			modbus_serial_disable(ctx);
		}
		break;
	case MODBUS_MODE_RAW:
		break;
	default:
		pr_err("Unknown MODBUS mode");
	}

	//TODO: FIXME
	//k_work_cancel_sync(&ctx->server_work, &work_sync);
	ctx->rxwait_to = 0;
	ctx->unit_id = 0;
	ctx->mbs_user_cb = NULL;
	atomic_clear_bit(&ctx->state, MODBUS_STATE_CONFIGURED);

	pr_info("Modbus interface %u disabled", iface);

	return 0;
}

static int modbus_core_init(void) {
	int err;
	
	/* 
	 * Initialize modbus service 
	 */
	err = tx_queue_create(&modbus_queue, (CHAR *)"modbus", sizeof(void *),
							(VOID *)modbus_queue_buffer, sizeof(modbus_queue_buffer));
	rte_assert(err == TX_SUCCESS);

	err = tx_thread_spawn(&modbus_thread, "modbus@rx", modbus_rx_server, NULL,
							modbus_stack, sizeof(modbus_stack),
							CONFIG_MMCSD_THREAD_PRIORITY, CONFIG_MMCSD_THREAD_PRIORITY,
							TX_NO_TIME_SLICE, TX_AUTO_START);
	rte_assert(err == TX_SUCCESS);

	LINKER_SET_FOREACH(modbus, ctx, struct modbus_context) {
		struct device *dev = device_find(ctx->cfg->name);
		rte_assert(dev != NULL);
		ctx->cfg->dev = dev;
	}

	return 0;
}

SYSINIT(modbus_core_init, SI_APPLICATION_LEVEL, 00);
