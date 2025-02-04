/*
 * Copyright 2024 wtcat
 */

#define TX_USE_BOARD_PRIVATE

#define pr_fmt(fmt) "[stm32_usb]: " fmt
#include "tx_api.h"
#include <errno.h>

#define UX_SOURCE_CODE
#include "ux_api.h"
#include "ux_device_stack.h"
#include "ux_device_class_storage.h"
#include "ux_utility.h"

#include "basework/log.h"

#include "stm32h7xx_hal_pcd.h"

#define UX_MEMORY_SIZE (28 * 1024)
#define _UX_ERR(_err) -(__ELASTERROR + (int)(_err))
#define STATIC_UINT static UINT

/* Define STM32 generic equivalences.  */

#define UX_DCD_STM32_SLAVE_CONTROLLER 0x80
#ifndef UX_DCD_STM32_MAX_ED
#define UX_DCD_STM32_MAX_ED 4
#endif /* UX_DCD_STM32_MAX_ED */
#define UX_DCD_STM32_IN_FIFO 3

#define UX_DCD_STM32_FLUSH_RX_FIFO 0x00000010
#define UX_DCD_STM32_FLUSH_TX_FIFO 0x00000020
#define UX_DCD_STM32_FLUSH_FIFO_ALL 0x00000010
#define UX_DCD_STM32_ENDPOINT_SPACE_SIZE 0x00000020
#define UX_DCD_STM32_ENDPOINT_CHANNEL_SIZE 0x00000020

/* Define USB STM32 physical endpoint status definition.  */

#define UX_DCD_STM32_ED_STATUS_UNUSED 0u
#define UX_DCD_STM32_ED_STATUS_USED 1u
#define UX_DCD_STM32_ED_STATUS_TRANSFER 2u
#define UX_DCD_STM32_ED_STATUS_STALLED 4u
#define UX_DCD_STM32_ED_STATUS_DONE 8u
#define UX_DCD_STM32_ED_STATUS_SETUP_IN (1u << 8)
#define UX_DCD_STM32_ED_STATUS_SETUP_STATUS (2u << 8)
#define UX_DCD_STM32_ED_STATUS_SETUP_OUT (3u << 8)
#define UX_DCD_STM32_ED_STATUS_SETUP (3u << 8)
#define UX_DCD_STM32_ED_STATUS_TASK_PENDING (1u << 10)

/* Define USB STM32 physical endpoint state machine definition.  */

#define UX_DCD_STM32_ED_STATE_IDLE 0
#define UX_DCD_STM32_ED_STATE_DATA_TX 1
#define UX_DCD_STM32_ED_STATE_DATA_RX 2
#define UX_DCD_STM32_ED_STATE_STATUS_TX 3
#define UX_DCD_STM32_ED_STATE_STATUS_RX 4

/* Define USB STM32 device callback notification state definition.  */

#define UX_DCD_STM32_SOF_RECEIVED 0xF0U
#define UX_DCD_STM32_DEVICE_CONNECTED 0xF1U
#define UX_DCD_STM32_DEVICE_DISCONNECTED 0xF2U
#define UX_DCD_STM32_DEVICE_RESUMED 0xF3U
#define UX_DCD_STM32_DEVICE_SUSPENDED 0xF4U

/* Define USB STM32 endpoint transfer status definition.  */

#define UX_DCD_STM32_ED_TRANSFER_STATUS_IDLE 0
#define UX_DCD_STM32_ED_TRANSFER_STATUS_SETUP 1
#define UX_DCD_STM32_ED_TRANSFER_STATUS_IN_COMPLETION 2
#define UX_DCD_STM32_ED_TRANSFER_STATUS_OUT_COMPLETION 3

/* Define USB STM32 physical endpoint structure.  */

typedef struct UX_DCD_STM32_ED_STRUCT {
	struct UX_SLAVE_ENDPOINT_STRUCT *ux_dcd_stm32_ed_endpoint;
	ULONG ux_dcd_stm32_ed_status;
	UCHAR ux_dcd_stm32_ed_state;
	UCHAR ux_dcd_stm32_ed_index;
	UCHAR ux_dcd_stm32_ed_direction;
	UCHAR reserved;
} UX_DCD_STM32_ED;

/* Define USB STM32 DCD structure definition.  */
typedef struct UX_DCD_STM32_STRUCT {
	struct UX_SLAVE_DCD_STRUCT *ux_dcd_stm32_dcd_owner;
	struct UX_DCD_STM32_ED_STRUCT ux_dcd_stm32_ed[UX_DCD_STM32_MAX_ED];
#if defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)
	struct UX_DCD_STM32_ED_STRUCT ux_dcd_stm32_ed_in[UX_DCD_STM32_MAX_ED];
#endif /* defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) */
	PCD_HandleTypeDef *pcd_handle;
} UX_DCD_STM32;

/* Local interface declare */
STATIC_UINT _ux_dcd_stm32_initialize_complete(VOID);
STATIC_UINT _ux_dcd_stm32_endpoint_stall(UX_DCD_STM32 *dcd_stm32, 
    UX_SLAVE_ENDPOINT *endpoint);


static inline struct UX_DCD_STM32_ED_STRUCT *
_stm32_ed_get(UX_DCD_STM32 *dcd_stm32, ULONG ep_addr) {
#if defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)
	ULONG ep_dir = ep_addr & 0x80u;
#endif /* defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) */
	ULONG ep_num = ep_addr & 0x7Fu;

	if (ep_num >= UX_DCD_STM32_MAX_ED ||
		ep_num >= dcd_stm32->pcd_handle->Init.dev_endpoints)
		return (UX_NULL);

#if defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)
	if (ep_dir)
		return (&dcd_stm32->ux_dcd_stm32_ed_in[ep_num]);
#endif /* defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) */

	return (&dcd_stm32->ux_dcd_stm32_ed[ep_num]);
}

static inline void 
_ux_dcd_stm32_setup_in(UX_DCD_STM32_ED *ed, UX_SLAVE_TRANSFER *transfer_request) {
	/* The endpoint is IN.  This is important to memorize the direction for the control
	   endpoint in case of a STALL. */
	ed->ux_dcd_stm32_ed_direction = UX_ENDPOINT_IN;

	/* Set the state to TX.  */
	ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_DATA_TX;

	/* Call the Control Transfer dispatcher.  */
	_ux_device_stack_control_request_process(transfer_request);
}

static inline void 
_ux_dcd_stm32_setup_out(UX_DCD_STM32_ED *ed, UX_SLAVE_TRANSFER *transfer_request, 
    PCD_HandleTypeDef *hpcd) {
	/* Set the completion code to no error.  */
	transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;

	/* The endpoint is IN.  This is important to memorize the direction for the control
	   endpoint in case of a STALL. */
	ed->ux_dcd_stm32_ed_direction = UX_ENDPOINT_IN;

	/* We are using a Control endpoint on a OUT transaction and there was a payload.  */
	if (_ux_device_stack_control_request_process(transfer_request) == UX_SUCCESS) {
		/* Set the state to STATUS phase TX.  */
		ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_STATUS_TX;

		/* Arm the status transfer.  */
		HAL_PCD_EP_Transmit(hpcd, 0x00U, UX_NULL, 0U);
	}
}

static inline void 
_ux_dcd_stm32_setup_status(UX_DCD_STM32_ED *ed, UX_SLAVE_TRANSFER *transfer_request, 
    PCD_HandleTypeDef *hpcd) {
	/* The endpoint is IN.  This is important to memorize the direction for the control
	   endpoint in case of a STALL. */
	ed->ux_dcd_stm32_ed_direction = UX_ENDPOINT_IN;

	/* Call the Control Transfer dispatcher.  */
	if (_ux_device_stack_control_request_process(transfer_request) == UX_SUCCESS) {
		/* Set the state to STATUS RX.  */
		ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_STATUS_RX;
		HAL_PCD_EP_Transmit(hpcd, 0x00U, UX_NULL, 0U);
	}
}

void HAL_PCD_SetupStageCallback(PCD_HandleTypeDef *hpcd) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_DCD_STM32_ED *ed;
	UX_SLAVE_TRANSFER *transfer_request;
	UX_SLAVE_ENDPOINT *endpoint;

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Fetch the address of the physical endpoint.  */
	ed = &dcd_stm32->ux_dcd_stm32_ed[0];

	/* Get the pointer to the transfer request.  */
	transfer_request = &ed->ux_dcd_stm32_ed_endpoint->ux_slave_endpoint_transfer_request;

	/* Copy setup data to transfer request.  */
	_ux_utility_memory_copy(transfer_request->ux_slave_transfer_request_setup,
							hpcd->Setup, UX_SETUP_SIZE);

	/* Clear the length of the data received.  */
	transfer_request->ux_slave_transfer_request_actual_length = 0;

	/* Mark the phase as SETUP.  */
	transfer_request->ux_slave_transfer_request_type = UX_TRANSFER_PHASE_SETUP;

	/* Mark the transfer as successful.  */
	transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;

	/* Set the status of the endpoint to not stalled.  */
	ed->ux_dcd_stm32_ed_status &=
		~(UX_DCD_STM32_ED_STATUS_STALLED | UX_DCD_STM32_ED_STATUS_TRANSFER |
		  UX_DCD_STM32_ED_STATUS_DONE);

	/* Check if the transaction is IN.  */
	if (*transfer_request->ux_slave_transfer_request_setup & UX_REQUEST_IN) {
#if defined(UX_DEVICE_STANDALONE)
		ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_SETUP_IN;
#else
		_ux_dcd_stm32_setup_in(ed, transfer_request);
#endif
	} else {
		/* The endpoint is OUT.  This is important to memorize the direction for the
		   control endpoint in case of a STALL. */
		ed->ux_dcd_stm32_ed_direction = UX_ENDPOINT_OUT;

		/* We are in a OUT transaction. Check if there is a data payload. If so, wait for
		   the payload to be delivered.  */
		if (*(transfer_request->ux_slave_transfer_request_setup + 6) == 0 &&
			*(transfer_request->ux_slave_transfer_request_setup + 7) == 0) {
#if defined(UX_DEVICE_STANDALONE)
			ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_SETUP_STATUS;
#else
			_ux_dcd_stm32_setup_status(ed, transfer_request, hpcd);
#endif
		} else {
			/* Get the pointer to the logical endpoint from the transfer request.  */
			endpoint = transfer_request->ux_slave_transfer_request_endpoint;

			/* Get the length we expect from the SETUP packet.  */
			transfer_request->ux_slave_transfer_request_requested_length =
				_ux_utility_short_get(transfer_request->ux_slave_transfer_request_setup +
									  6);

			/* Check if we have enough space for the request.  */
			if (transfer_request->ux_slave_transfer_request_requested_length >
				UX_SLAVE_REQUEST_CONTROL_MAX_LENGTH) {
				/* No space available, stall the endpoint.  */
				_ux_dcd_stm32_endpoint_stall(dcd_stm32, endpoint);

				/* Next phase is a SETUP.  */
				ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_IDLE;

#if defined(UX_DEVICE_STANDALONE)
				ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_SETUP_STATUS;
#endif

				/* We are done.  */
				return;
			} else {
				/* Reset what we have received so far.  */
				transfer_request->ux_slave_transfer_request_actual_length = 0;

				/* And reprogram the current buffer address to the beginning of the
				 * buffer.  */
				transfer_request->ux_slave_transfer_request_current_data_pointer =
					transfer_request->ux_slave_transfer_request_data_pointer;

				/* Receive data.  */
				HAL_PCD_EP_Receive(
					hpcd, endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
					transfer_request->ux_slave_transfer_request_current_data_pointer,
					transfer_request->ux_slave_transfer_request_requested_length);

				/* Set the state to RX.  */
				ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_DATA_RX;
			}
		}
	}
}

void HAL_PCD_DataInStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_DCD_STM32_ED *ed;
	UX_SLAVE_TRANSFER *transfer_request;
	ULONG transfer_length;
	UX_SLAVE_ENDPOINT *endpoint;

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Fetch the address of the physical endpoint.  */
#if defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)
	if ((epnum & 0xF) != 0)
		ed = &dcd_stm32->ux_dcd_stm32_ed_in[epnum & 0xF];
	else
#endif /* defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) */
		ed = &dcd_stm32->ux_dcd_stm32_ed[epnum & 0xF];

	/* Get the pointer to the transfer request.  */
	transfer_request =
		&(ed->ux_dcd_stm32_ed_endpoint->ux_slave_endpoint_transfer_request);

	/* Endpoint 0 is different.  */
	if (epnum == 0U) {
		/* Get the pointer to the logical endpoint from the transfer request.  */
		endpoint = transfer_request->ux_slave_transfer_request_endpoint;

		/* Check if we need to send data again on control endpoint. */
		if (ed->ux_dcd_stm32_ed_state == UX_DCD_STM32_ED_STATE_DATA_TX) {
			/* Arm Status transfer.  */
			HAL_PCD_EP_Receive(hpcd, 0, 0, 0);

			/* Are we done with this transfer ? */
			if (transfer_request->ux_slave_transfer_request_in_transfer_length <=
				endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize) {
				/* There is no data to send but we may need to send a Zero Length Packet.
				 */
				if (transfer_request->ux_slave_transfer_request_force_zlp == UX_TRUE) {
					/* Arm a ZLP packet on IN.  */
					HAL_PCD_EP_Transmit(
						hpcd, endpoint->ux_slave_endpoint_descriptor.bEndpointAddress, 0,
						0);

					/* Reset the ZLP condition.  */
					transfer_request->ux_slave_transfer_request_force_zlp = UX_FALSE;

				} else {
					/* Set the completion code to no error.  */
					transfer_request->ux_slave_transfer_request_completion_code =
						UX_SUCCESS;

					/* The transfer is completed.  */
					transfer_request->ux_slave_transfer_request_status =
						UX_TRANSFER_STATUS_COMPLETED;
					transfer_request->ux_slave_transfer_request_actual_length =
						transfer_request->ux_slave_transfer_request_requested_length;

#if defined(UX_DEVICE_STANDALONE)

					/* Control status phase done.  */
					ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_DONE;
#endif

					/* We are using a Control endpoint, if there is a callback, invoke it.
					 * We are still under ISR.  */
					if (transfer_request->ux_slave_transfer_request_completion_function)
						transfer_request->ux_slave_transfer_request_completion_function(
							transfer_request);

					/* State is now STATUS RX.  */
					ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_STATUS_RX;
				}
			} else {
				/* Get the size of the transfer.  */
				transfer_length =
					transfer_request->ux_slave_transfer_request_in_transfer_length -
					endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize;

				/* Check if the endpoint size is bigger that data requested. */
				if (transfer_length >
					endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize) {
					/* Adjust the transfer size.  */
					transfer_length =
						endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize;
				}

				/* Adjust the data pointer.  */
				transfer_request->ux_slave_transfer_request_current_data_pointer +=
					endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize;

				/* Adjust the transfer length remaining.  */
				transfer_request->ux_slave_transfer_request_in_transfer_length -=
					transfer_length;

				/* Transmit data.  */
				HAL_PCD_EP_Transmit(
					hpcd, endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
					transfer_request->ux_slave_transfer_request_current_data_pointer,
					transfer_length);
			}
		}
	} else {
		/* Check if a ZLP should be armed.  */
		if (transfer_request->ux_slave_transfer_request_force_zlp &&
			transfer_request->ux_slave_transfer_request_requested_length) {
			/* Reset the ZLP condition.  */
			transfer_request->ux_slave_transfer_request_force_zlp = UX_FALSE;
			transfer_request->ux_slave_transfer_request_in_transfer_length = 0;

			/* Arm a ZLP packet on IN.  */
			HAL_PCD_EP_Transmit(hpcd, epnum, 0, 0);

		} else {
			/* Set the completion code to no error.  */
			transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;

			/* The transfer is completed.  */
			transfer_request->ux_slave_transfer_request_status =
				UX_TRANSFER_STATUS_COMPLETED;
			transfer_request->ux_slave_transfer_request_actual_length =
				transfer_request->ux_slave_transfer_request_requested_length;

#if defined(UX_DEVICE_STANDALONE)
			ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_DONE;
#else

			/* Non control endpoint operation, use semaphore.  */
			_ux_utility_semaphore_put(
				&transfer_request->ux_slave_transfer_request_semaphore);
#endif /* defined(UX_DEVICE_STANDALONE) */
		}
	}
}

void HAL_PCD_DataOutStageCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_DCD_STM32_ED *ed;
	UX_SLAVE_TRANSFER *transfer_request;
	ULONG transfer_length;
	UX_SLAVE_ENDPOINT *endpoint;

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Fetch the address of the physical endpoint.  */
	ed = &dcd_stm32->ux_dcd_stm32_ed[epnum & 0xF];

	/* Get the pointer to the transfer request.  */
	transfer_request =
		&(ed->ux_dcd_stm32_ed_endpoint->ux_slave_endpoint_transfer_request);

	/* Endpoint 0 is different.  */
	if (epnum == 0U) {
		/* Check if we have received something on endpoint 0 during data phase .  */
		if (ed->ux_dcd_stm32_ed_state == UX_DCD_STM32_ED_STATE_DATA_RX) {
			/* Get the pointer to the logical endpoint from the transfer request.  */
			endpoint = transfer_request->ux_slave_transfer_request_endpoint;

			/* Read the received data length for the Control endpoint.  */
			transfer_length = HAL_PCD_EP_GetRxCount(hpcd, epnum);

			/* Update the length of the data received.  */
			transfer_request->ux_slave_transfer_request_actual_length += transfer_length;

			/* Can we accept this much?  */
			if (transfer_request->ux_slave_transfer_request_actual_length <=
				transfer_request->ux_slave_transfer_request_requested_length) {
				/* Are we done with this transfer ? */
				if ((transfer_request->ux_slave_transfer_request_actual_length ==
					 transfer_request->ux_slave_transfer_request_requested_length) ||
					(transfer_length !=
					 endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize)) {
#if defined(UX_DEVICE_STANDALONE)
					ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_SETUP_OUT;
#else
					_ux_dcd_stm32_setup_out(ed, transfer_request, hpcd);
#endif
				} else {
					/* Rearm the OUT control endpoint for one packet. */
					transfer_request->ux_slave_transfer_request_current_data_pointer +=
						endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize;
					HAL_PCD_EP_Receive(
						hpcd, endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
						transfer_request->ux_slave_transfer_request_current_data_pointer,
						endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize);
				}
			} else {
				/*  We have an overflow situation. Set the completion code to overflow. */
				transfer_request->ux_slave_transfer_request_completion_code =
					UX_TRANSFER_BUFFER_OVERFLOW;

				/* If trace is enabled, insert this event into the trace buffer.  */
				UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_TRANSFER_BUFFER_OVERFLOW,
										transfer_request, 0, 0, UX_TRACE_ERRORS, 0, 0)

#if defined(UX_DEVICE_STANDALONE)

				/* Control status phase done.  */
				ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_DONE;
#endif

				/* We are using a Control endpoint, if there is a callback, invoke it. We
				 * are still under ISR.  */
				if (transfer_request->ux_slave_transfer_request_completion_function)
					transfer_request->ux_slave_transfer_request_completion_function(
						transfer_request);
			}
		}
	} else {
		/* Update the length of the data sent in previous transaction.  */
		transfer_request->ux_slave_transfer_request_actual_length =
			HAL_PCD_EP_GetRxCount(hpcd, epnum);

		/* Set the completion code to no error.  */
		transfer_request->ux_slave_transfer_request_completion_code = UX_SUCCESS;

		/* The transfer is completed.  */
		transfer_request->ux_slave_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;

#if defined(UX_DEVICE_STANDALONE)
		ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_DONE;
#else

		/* Non control endpoint operation, use semaphore.  */
		_ux_utility_semaphore_put(&transfer_request->ux_slave_transfer_request_semaphore);
#endif
	}
}

void HAL_PCD_ResetCallback(PCD_HandleTypeDef *hpcd) {
	/* If the device is attached or configured, we need to disconnect it.  */
	if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state !=
		UX_DEVICE_RESET) {
		/* Disconnect the device.  */
		_ux_device_stack_disconnect();
	}

	/* Set USB Current Speed */
	switch (hpcd->Init.speed) {
#ifdef PCD_SPEED_HIGH
	case PCD_SPEED_HIGH:

		/* We are connected at high speed.  */
		_ux_system_slave->ux_system_slave_speed = UX_HIGH_SPEED_DEVICE;
		break;
#endif
	case PCD_SPEED_FULL:

		/* We are connected at full speed.  */
		_ux_system_slave->ux_system_slave_speed = UX_FULL_SPEED_DEVICE;
		break;

	default:

		/* We are connected at full speed.  */
		_ux_system_slave->ux_system_slave_speed = UX_FULL_SPEED_DEVICE;
		break;
	}

	/* Complete the device initialization.  */
	_ux_dcd_stm32_initialize_complete();

	/* Mark the device as attached now.  */
	_ux_system_slave->ux_system_slave_device.ux_slave_device_state = UX_DEVICE_ATTACHED;
}

void HAL_PCD_ConnectCallback(PCD_HandleTypeDef *hpcd) {
	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(UX_DCD_STM32_DEVICE_CONNECTED);
	}
}

void HAL_PCD_DisconnectCallback(PCD_HandleTypeDef *hpcd) {
	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(
			UX_DCD_STM32_DEVICE_DISCONNECTED);
	}

	/* Check if the device is attached or configured.  */
	if (_ux_system_slave->ux_system_slave_device.ux_slave_device_state !=
		UX_DEVICE_RESET) {
		/* Disconnect the device.  */
		_ux_device_stack_disconnect();
	}
}

void HAL_PCD_SuspendCallback(PCD_HandleTypeDef *hpcd) {
	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(UX_DCD_STM32_DEVICE_SUSPENDED);
	}
}

void HAL_PCD_ResumeCallback(PCD_HandleTypeDef *hpcd) {
	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(UX_DCD_STM32_DEVICE_RESUMED);
	}
}

void HAL_PCD_SOFCallback(PCD_HandleTypeDef *hpcd) {
	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(UX_DCD_STM32_SOF_RECEIVED);
	}
}

void HAL_PCD_ISOINIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_DCD_STM32_ED *ed;
	UX_SLAVE_ENDPOINT *endpoint;

	UX_PARAMETER_NOT_USED(epnum);

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

#if defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT)
	ed = &dcd_stm32->ux_dcd_stm32_ed_in[epnum & 0xF];
#else
	ed = &dcd_stm32->ux_dcd_stm32_ed[epnum & 0xF];
#endif /* defined(UX_DEVICE_BIDIRECTIONAL_ENDPOINT_SUPPORT) */

	if ((ed->ux_dcd_stm32_ed_status & UX_DCD_STM32_ED_STATUS_USED) == 0U)
		return;

	endpoint = ed->ux_dcd_stm32_ed_endpoint;

	if ((endpoint->ux_slave_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) ==
			1 &&
		(endpoint->ux_slave_endpoint_descriptor.bEndpointAddress &
		 UX_ENDPOINT_DIRECTION) != 0) {
		/* Incomplete, discard data and retry.  */
		HAL_PCD_EP_Transmit(dcd_stm32->pcd_handle,
							endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
							endpoint->ux_slave_endpoint_transfer_request
								.ux_slave_transfer_request_data_pointer,
							endpoint->ux_slave_endpoint_transfer_request
								.ux_slave_transfer_request_requested_length);
	}
}

void HAL_PCD_ISOOUTIncompleteCallback(PCD_HandleTypeDef *hpcd, uint8_t epnum) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_DCD_STM32_ED *ed;
	UX_SLAVE_ENDPOINT *endpoint;

	UX_PARAMETER_NOT_USED(epnum);

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	ed = &dcd_stm32->ux_dcd_stm32_ed[epnum & 0xF];
	if ((ed->ux_dcd_stm32_ed_status & UX_DCD_STM32_ED_STATUS_USED) == 0)
		return;

	endpoint = ed->ux_dcd_stm32_ed_endpoint;

	if ((endpoint->ux_slave_endpoint_descriptor.bmAttributes & UX_MASK_ENDPOINT_TYPE) ==
			1 &&
		(endpoint->ux_slave_endpoint_descriptor.bEndpointAddress &
		 UX_ENDPOINT_DIRECTION) == 0) {
		/* Incomplete, discard data and retry.  */
		HAL_PCD_EP_Receive(dcd_stm32->pcd_handle,
						   endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
						   endpoint->ux_slave_endpoint_transfer_request
							   .ux_slave_transfer_request_data_pointer,
						   endpoint->ux_slave_endpoint_transfer_request
							   .ux_slave_transfer_request_requested_length);
	}
}

STATIC_UINT _ux_dcd_stm32_endpoint_create(UX_DCD_STM32 *dcd_stm32, 
    UX_SLAVE_ENDPOINT *endpoint) {
	UX_DCD_STM32_ED *ed;
	ULONG stm32_endpoint_index;

	/* The endpoint index in the array of the STM32 must match the endpoint number.  */
	stm32_endpoint_index =
		endpoint->ux_slave_endpoint_descriptor.bEndpointAddress & ~UX_ENDPOINT_DIRECTION;

	/* Get STM32 ED.  */
	ed =
		_stm32_ed_get(dcd_stm32, endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);

	if (ed == UX_NULL)
		return (UX_NO_ED_AVAILABLE);

	/* Check the endpoint status, if it is free, reserve it. If not reject this endpoint.
	 */
	if ((ed->ux_dcd_stm32_ed_status & UX_DCD_STM32_ED_STATUS_USED) == 0) {
		/* We can use this endpoint.  */
		ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_USED;

		/* Keep the physical endpoint address in the endpoint container.  */
		endpoint->ux_slave_endpoint_ed = (VOID *)ed;

		/* Save the endpoint pointer.  */
		ed->ux_dcd_stm32_ed_endpoint = endpoint;

		/* And its index.  */
		ed->ux_dcd_stm32_ed_index = stm32_endpoint_index;

		/* And its direction.  */
		ed->ux_dcd_stm32_ed_direction =
			endpoint->ux_slave_endpoint_descriptor.bEndpointAddress &
			UX_ENDPOINT_DIRECTION;

		/* Check if it is non-control endpoint.  */
		if (stm32_endpoint_index != 0) {
			/* Open the endpoint.  */
			HAL_PCD_EP_Open(dcd_stm32->pcd_handle,
							endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
							endpoint->ux_slave_endpoint_descriptor.wMaxPacketSize,
							endpoint->ux_slave_endpoint_descriptor.bmAttributes &
								UX_MASK_ENDPOINT_TYPE);
		}

		/* Return successful completion.  */
		return (UX_SUCCESS);
	}

	/* Return an error.  */
	return (UX_NO_ED_AVAILABLE);
}

STATIC_UINT _ux_dcd_stm32_endpoint_destroy(UX_DCD_STM32 *dcd_stm32,
									UX_SLAVE_ENDPOINT *endpoint) {
	UX_DCD_STM32_ED *ed;

	/* Keep the physical endpoint address in the endpoint container.  */
	ed = (UX_DCD_STM32_ED *)endpoint->ux_slave_endpoint_ed;

	/* We can free this endpoint.  */
	ed->ux_dcd_stm32_ed_status = UX_DCD_STM32_ED_STATUS_UNUSED;

	/* Deactivate the endpoint.  */
	HAL_PCD_EP_Close(dcd_stm32->pcd_handle,
					 endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);

	/* This function never fails.  */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_endpoint_reset(UX_DCD_STM32 *dcd_stm32, 
    UX_SLAVE_ENDPOINT *endpoint) {
	UX_INTERRUPT_SAVE_AREA
	UX_DCD_STM32_ED *ed;

	/* Get the physical endpoint address in the endpoint container.  */
	ed = (UX_DCD_STM32_ED *)endpoint->ux_slave_endpoint_ed;

	UX_DISABLE

	/* Set the status of the endpoint to not stalled.  */
	ed->ux_dcd_stm32_ed_status &=
		~(UX_DCD_STM32_ED_STATUS_STALLED | UX_DCD_STM32_ED_STATUS_DONE |
		  UX_DCD_STM32_ED_STATUS_SETUP);

	/* Set the state of the endpoint to IDLE.  */
	ed->ux_dcd_stm32_ed_state = UX_DCD_STM32_ED_STATE_IDLE;

	/* Clear STALL condition.  */
	HAL_PCD_EP_ClrStall(dcd_stm32->pcd_handle,
						endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);

	/* Flush buffer.  */
	HAL_PCD_EP_Flush(dcd_stm32->pcd_handle,
					 endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);

#ifndef UX_DEVICE_STANDALONE

	/* Wakeup pending thread.  */
	if (endpoint->ux_slave_endpoint_transfer_request.ux_slave_transfer_request_semaphore
			.tx_semaphore_suspended_count)
		_ux_utility_semaphore_put(&endpoint->ux_slave_endpoint_transfer_request
									   .ux_slave_transfer_request_semaphore);
#endif

	UX_RESTORE

	/* This function never fails.  */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_endpoint_stall(UX_DCD_STM32 *dcd_stm32, 
    UX_SLAVE_ENDPOINT *endpoint) {
	UX_DCD_STM32_ED *ed;

	/* Get the physical endpoint address in the endpoint container.  */
	ed = (UX_DCD_STM32_ED *)endpoint->ux_slave_endpoint_ed;

	/* Set the endpoint to stall.  */
	ed->ux_dcd_stm32_ed_status |= UX_DCD_STM32_ED_STATUS_STALLED;

	/* Stall the endpoint.  */
	HAL_PCD_EP_SetStall(dcd_stm32->pcd_handle,
						endpoint->ux_slave_endpoint_descriptor.bEndpointAddress |
							ed->ux_dcd_stm32_ed_direction);

	/* This function never fails.  */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_endpoint_status(UX_DCD_STM32 *dcd_stm32, 
    ULONG endpoint_index) {
	UX_DCD_STM32_ED *ed;

	/* Fetch the address of the physical endpoint.  */
	ed = _stm32_ed_get(dcd_stm32, endpoint_index);

	/* Check the endpoint status, if it is free, we have a illegal endpoint.  */
	if ((ed->ux_dcd_stm32_ed_status & UX_DCD_STM32_ED_STATUS_USED) == 0)
		return (UX_ERROR);

	/* Check if the endpoint is stalled.  */
	if ((ed->ux_dcd_stm32_ed_status & UX_DCD_STM32_ED_STATUS_STALLED) == 0)
		return (UX_FALSE);
	else
		return (UX_TRUE);
}

STATIC_UINT _ux_dcd_stm32_frame_number_get(UX_DCD_STM32 *dcd_stm32, 
    ULONG *frame_number) {
	/* This function never fails. */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_transfer_abort(UX_DCD_STM32 *dcd_stm32,
								  UX_SLAVE_TRANSFER *transfer_request) {
#if !defined(USBD_HAL_TRANSFER_ABORT_NOT_SUPPORTED)

	UX_SLAVE_ENDPOINT *endpoint;

	/* Get the pointer to the logical endpoint from the transfer request.  */
	endpoint = transfer_request->ux_slave_transfer_request_endpoint;

	HAL_PCD_EP_Abort(dcd_stm32->pcd_handle,
					 endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);
	HAL_PCD_EP_Flush(dcd_stm32->pcd_handle,
					 endpoint->ux_slave_endpoint_descriptor.bEndpointAddress);

	/* No semaphore put here since it's already done in stack.  */
#endif /* USBD_HAL_TRANSFER_ABORT_NOT_SUPPORTED */

	/* Return to caller with success.  */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_transfer_request(UX_DCD_STM32 *dcd_stm32,
									UX_SLAVE_TRANSFER *transfer_request) {
	UX_SLAVE_ENDPOINT *endpoint;
	UINT status;

	/* Get the pointer to the logical endpoint from the transfer request.  */
	endpoint = transfer_request->ux_slave_transfer_request_endpoint;

	/* Check for transfer direction.  Is this a IN endpoint ? */
	if (transfer_request->ux_slave_transfer_request_phase == UX_TRANSFER_PHASE_DATA_OUT) {
		/* Transmit data.  */
		HAL_PCD_EP_Transmit(dcd_stm32->pcd_handle,
							endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
							transfer_request->ux_slave_transfer_request_data_pointer,
							transfer_request->ux_slave_transfer_request_requested_length);

		/* If the endpoint is a Control endpoint, all this is happening under Interrupt
		   and there is no thread to suspend.  */
		if ((endpoint->ux_slave_endpoint_descriptor.bEndpointAddress &
			 (UINT)~UX_ENDPOINT_DIRECTION) != 0) {
			/* We should wait for the semaphore to wake us up.  */
			status = _ux_utility_semaphore_get(
				&transfer_request->ux_slave_transfer_request_semaphore,
				(ULONG)transfer_request->ux_slave_transfer_request_timeout);

			/* Check the completion code. */
			if (status != UX_SUCCESS)
				return (status);

			transfer_request->ux_slave_transfer_request_actual_length =
				transfer_request->ux_slave_transfer_request_requested_length;

			/* Check the transfer request completion code. We may have had a BUS reset or
			   a device disconnection.  */
			if (transfer_request->ux_slave_transfer_request_completion_code != UX_SUCCESS)
				return (transfer_request->ux_slave_transfer_request_completion_code);

			/* Return to caller with success.  */
			return (UX_SUCCESS);
		}
	} else {
		/* We have a request for a SETUP or OUT Endpoint.  */
		/* Receive data.  */
		HAL_PCD_EP_Receive(dcd_stm32->pcd_handle,
						   endpoint->ux_slave_endpoint_descriptor.bEndpointAddress,
						   transfer_request->ux_slave_transfer_request_data_pointer,
						   transfer_request->ux_slave_transfer_request_requested_length);

		/* If the endpoint is a Control endpoint, all this is happening under Interrupt
		   and there is no thread to suspend.  */
		if ((endpoint->ux_slave_endpoint_descriptor.bEndpointAddress &
			 (UINT)~UX_ENDPOINT_DIRECTION) != 0) {
			/* We should wait for the semaphore to wake us up.  */
			status = _ux_utility_semaphore_get(
				&transfer_request->ux_slave_transfer_request_semaphore,
				(ULONG)transfer_request->ux_slave_transfer_request_timeout);

			/* Check the completion code. */
			if (status != UX_SUCCESS)
				return (status);

			/* Check the transfer request completion code. We may have had a BUS reset or
			   a device disconnection.  */
			if (transfer_request->ux_slave_transfer_request_completion_code != UX_SUCCESS)
				return (transfer_request->ux_slave_transfer_request_completion_code);

			/* Return to caller with success.  */
			return (UX_SUCCESS);
		}
	}

	/* Return to caller with success.  */
	return (UX_SUCCESS);
}

STATIC_UINT _ux_dcd_stm32_function(UX_SLAVE_DCD *dcd, UINT function, VOID *parameter) {
	UINT status;
	UX_DCD_STM32 *dcd_stm32;

	/* Check the status of the controller.  */
	if (dcd->ux_slave_dcd_status == UX_UNUSED) {
		/* Error trap. */
		_ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_DCD,
								 UX_CONTROLLER_UNKNOWN);

		/* If trace is enabled, insert this event into the trace buffer.  */
		UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_CONTROLLER_UNKNOWN, 0, 0, 0,
								UX_TRACE_ERRORS, 0, 0)

		return (UX_CONTROLLER_UNKNOWN);
	}

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Look at the function and route it.  */
	switch (function) {
	case UX_DCD_GET_FRAME_NUMBER:

		status = _ux_dcd_stm32_frame_number_get(dcd_stm32, (ULONG *)parameter);
		break;

	case UX_DCD_TRANSFER_REQUEST:

#if defined(UX_DEVICE_STANDALONE)
		status = _ux_dcd_stm32_transfer_run(dcd_stm32, (UX_SLAVE_TRANSFER *)parameter);
#else
		status =
			_ux_dcd_stm32_transfer_request(dcd_stm32, (UX_SLAVE_TRANSFER *)parameter);
#endif /* defined(UX_DEVICE_STANDALONE) */
		break;

	case UX_DCD_TRANSFER_ABORT:
		status = _ux_dcd_stm32_transfer_abort(dcd_stm32, parameter);
		break;

	case UX_DCD_CREATE_ENDPOINT:

		status = _ux_dcd_stm32_endpoint_create(dcd_stm32, parameter);
		break;

	case UX_DCD_DESTROY_ENDPOINT:

		status = _ux_dcd_stm32_endpoint_destroy(dcd_stm32, parameter);
		break;

	case UX_DCD_RESET_ENDPOINT:

		status = _ux_dcd_stm32_endpoint_reset(dcd_stm32, parameter);
		break;

	case UX_DCD_STALL_ENDPOINT:

		status = _ux_dcd_stm32_endpoint_stall(dcd_stm32, parameter);
		break;

	case UX_DCD_SET_DEVICE_ADDRESS:

		status = HAL_PCD_SetAddress(dcd_stm32->pcd_handle, (uint8_t)(ULONG)parameter);
		break;

	case UX_DCD_CHANGE_STATE:

		if ((ULONG)parameter == UX_DEVICE_FORCE_DISCONNECT) {
			/* Disconnect the USB device */
			status = HAL_PCD_Stop(dcd_stm32->pcd_handle);
		} else {
			status = UX_SUCCESS;
		}

		break;

	case UX_DCD_ENDPOINT_STATUS:

		status = _ux_dcd_stm32_endpoint_status(dcd_stm32, (ULONG)parameter);
		break;

#if defined(UX_DEVICE_STANDALONE)
	case UX_DCD_ISR_PENDING:

		_ux_dcd_stm32_setup_isr_pending(dcd_stm32);
		status = UX_SUCCESS;
		break;
#endif /* defined(UX_DEVICE_STANDALONE) */

	default:

		/* Error trap. */
		_ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_DCD,
								 UX_FUNCTION_NOT_SUPPORTED);

		/* If trace is enabled, insert this event into the trace buffer.  */
		UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0,
								UX_TRACE_ERRORS, 0, 0)

		status = UX_FUNCTION_NOT_SUPPORTED;
		break;
	}

	/* Return completion status.  */
	return (status);
}

STATIC_UINT _ux_dcd_stm32_initialize_complete(VOID) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;
	UX_SLAVE_DEVICE *device;
	UCHAR *device_framework;
	UX_SLAVE_TRANSFER *transfer_request;

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Get the pointer to the device.  */
	device = &_ux_system_slave->ux_system_slave_device;

	/* Are we in DFU mode ? If so, check if we are in a Reset mode.  */
	if (_ux_system_slave->ux_system_slave_device_dfu_state_machine ==
		UX_SYSTEM_DFU_STATE_APP_DETACH) {
		/* The device is now in DFU reset mode. Switch to the DFU device framework.  */
		_ux_system_slave->ux_system_slave_device_framework =
			_ux_system_slave->ux_system_slave_dfu_framework;
		_ux_system_slave->ux_system_slave_device_framework_length =
			_ux_system_slave->ux_system_slave_dfu_framework_length;

	} else {
		/* Set State to App Idle. */
		_ux_system_slave->ux_system_slave_device_dfu_state_machine =
			UX_SYSTEM_DFU_STATE_APP_IDLE;

		/* Check the speed and set the correct descriptor.  */
		if (_ux_system_slave->ux_system_slave_speed == UX_FULL_SPEED_DEVICE) {
			/* The device is operating at full speed.  */
			_ux_system_slave->ux_system_slave_device_framework =
				_ux_system_slave->ux_system_slave_device_framework_full_speed;
			_ux_system_slave->ux_system_slave_device_framework_length =
				_ux_system_slave->ux_system_slave_device_framework_length_full_speed;
		} else {
			/* The device is operating at high speed.  */
			_ux_system_slave->ux_system_slave_device_framework =
				_ux_system_slave->ux_system_slave_device_framework_high_speed;
			_ux_system_slave->ux_system_slave_device_framework_length =
				_ux_system_slave->ux_system_slave_device_framework_length_high_speed;
		}
	}

	/* Get the device framework pointer.  */
	device_framework = _ux_system_slave->ux_system_slave_device_framework;

	/* And create the decompressed device descriptor structure.  */
	_ux_utility_descriptor_parse(device_framework, _ux_system_device_descriptor_structure,
								 UX_DEVICE_DESCRIPTOR_ENTRIES,
								 (UCHAR *)&device->ux_slave_device_descriptor);

	/* Now we create a transfer request to accept the first SETUP packet
	   and get the ball running. First get the address of the endpoint
	   transfer request container.  */
	transfer_request =
		&device->ux_slave_device_control_endpoint.ux_slave_endpoint_transfer_request;

	/* Set the timeout to be for Control Endpoint.  */
	transfer_request->ux_slave_transfer_request_timeout =
		UX_MS_TO_TICK(UX_CONTROL_TRANSFER_TIMEOUT);

	/* Adjust the current data pointer as well.  */
	transfer_request->ux_slave_transfer_request_current_data_pointer =
		transfer_request->ux_slave_transfer_request_data_pointer;

	/* Update the transfer request endpoint pointer with the default endpoint.  */
	transfer_request->ux_slave_transfer_request_endpoint =
		&device->ux_slave_device_control_endpoint;

	/* The control endpoint max packet size needs to be filled manually in its descriptor.
	 */
	transfer_request->ux_slave_transfer_request_endpoint->ux_slave_endpoint_descriptor
		.wMaxPacketSize = device->ux_slave_device_descriptor.bMaxPacketSize0;

	/* On the control endpoint, always expect the maximum.  */
	transfer_request->ux_slave_transfer_request_requested_length =
		device->ux_slave_device_descriptor.bMaxPacketSize0;

	/* Attach the control endpoint to the transfer request.  */
	transfer_request->ux_slave_transfer_request_endpoint =
		&device->ux_slave_device_control_endpoint;

	/* Create the default control endpoint attached to the device.
	   Once this endpoint is enabled, the host can then send a setup packet
	   The device controller will receive it and will call the setup function
	   module.  */
	dcd->ux_slave_dcd_function(dcd, UX_DCD_CREATE_ENDPOINT,
							   (VOID *)&device->ux_slave_device_control_endpoint);

	/* Open Control OUT endpoint.  */
	HAL_PCD_EP_Flush(dcd_stm32->pcd_handle, 0x00U);
	HAL_PCD_EP_Open(dcd_stm32->pcd_handle, 0x00U,
					device->ux_slave_device_descriptor.bMaxPacketSize0,
					UX_CONTROL_ENDPOINT);

	/* Open Control IN endpoint.  */
	HAL_PCD_EP_Flush(dcd_stm32->pcd_handle, 0x80U);
	HAL_PCD_EP_Open(dcd_stm32->pcd_handle, 0x80U,
					device->ux_slave_device_descriptor.bMaxPacketSize0,
					UX_CONTROL_ENDPOINT);

	/* Ensure the control endpoint is properly reset.  */
	device->ux_slave_device_control_endpoint.ux_slave_endpoint_state = UX_ENDPOINT_RESET;

	/* Mark the phase as SETUP.  */
	transfer_request->ux_slave_transfer_request_type = UX_TRANSFER_PHASE_SETUP;

	/* Mark this transfer request as pending.  */
	transfer_request->ux_slave_transfer_request_status = UX_TRANSFER_STATUS_PENDING;

	/* Ask for 8 bytes of the SETUP packet.  */
	transfer_request->ux_slave_transfer_request_requested_length = UX_SETUP_SIZE;
	transfer_request->ux_slave_transfer_request_in_transfer_length = UX_SETUP_SIZE;

	/* Reset the number of bytes sent/received.  */
	transfer_request->ux_slave_transfer_request_actual_length = 0;

	/* Check the status change callback.  */
	if (_ux_system_slave->ux_system_slave_change_function != UX_NULL) {
		/* Inform the application if a callback function was programmed.  */
		_ux_system_slave->ux_system_slave_change_function(UX_DEVICE_ATTACHED);
	}

	/* If trace is enabled, insert this event into the trace buffer.  */
	UX_TRACE_IN_LINE_INSERT(UX_TRACE_DEVICE_STACK_CONNECT, 0, 0, 0, 0,
							UX_TRACE_DEVICE_STACK_EVENTS, 0, 0)

	/* If trace is enabled, register this object.  */
	UX_TRACE_OBJECT_REGISTER(UX_TRACE_DEVICE_OBJECT_TYPE_DEVICE, device, 0, 0, 0)

	/* We are now ready for the USB device to accept the first packet when connected.  */
	return (UX_SUCCESS);
}

static VOID _ux_dcd_stm32_interrupt_handler(VOID) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* Get the pointer to the STM32 DCD.  */
	dcd_stm32 = (UX_DCD_STM32 *)dcd->ux_slave_dcd_controller_hardware;

	/* Call the actual interrupt handler function.  */
	HAL_PCD_IRQHandler(dcd_stm32->pcd_handle);
}

STATIC_UINT _ux_dcd_stm32_initialize(ULONG dcd_io, ULONG parameter) {
	UX_SLAVE_DCD *dcd;
	UX_DCD_STM32 *dcd_stm32;

	UX_PARAMETER_NOT_USED(dcd_io);

	/* Get the pointer to the DCD.  */
	dcd = &_ux_system_slave->ux_system_slave_dcd;

	/* The controller initialized here is of STM32 type.  */
	dcd->ux_slave_dcd_controller_type = UX_DCD_STM32_SLAVE_CONTROLLER;

	/* Allocate memory for this STM32 DCD instance.  */
	dcd_stm32 =
		_ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_DCD_STM32));

	/* Check if memory was properly allocated.  */
	if (dcd_stm32 == UX_NULL)
		return (UX_MEMORY_INSUFFICIENT);

	/* Set the pointer to the STM32 DCD.  */
	dcd->ux_slave_dcd_controller_hardware = (VOID *)dcd_stm32;

	/* Set the generic DCD owner for the STM32 DCD.  */
	dcd_stm32->ux_dcd_stm32_dcd_owner = dcd;

	/* Initialize the function collector for this DCD.  */
	dcd->ux_slave_dcd_function = _ux_dcd_stm32_function;

	dcd_stm32->pcd_handle = (PCD_HandleTypeDef *)parameter;

	/* Set the state of the controller to OPERATIONAL now.  */
	dcd->ux_slave_dcd_status = UX_DCD_STATUS_OPERATIONAL;

	/* Return successful completion.  */
	return (UX_SUCCESS);
}

static TX_SEMAPHORE dcd_irq_signal;
static TX_THREAD    dcd_irq_thread;
static ULONG        dcd_irq_stack[1024 / sizeof(ULONG)];
static PCD_HandleTypeDef usb_device_handle = {
	.Instance = USB_OTG_FS,
	.Init = {
		.dev_endpoints = UX_DCD_STM32_MAX_ED,
		.Host_channels = UX_DCD_STM32_MAX_ED,
		.speed = USBD_FS_SPEED,
		.ep0_mps = 0,
		.phy_itface = USB_OTG_EMBEDDED_PHY,
		.Sof_enable = 0,
		.lpm_enable = 0,
		.battery_charging_enable = 0,
		.vbus_sensing_enable = 0,
		.use_dedicated_ep1 = 0,
		.use_external_vbus = 0
	}
};

#define DEVICE_FRAMEWORK_LENGTH_FULL_SPEED 50
static UCHAR device_framework_full_speed[] = { 
    /* Device descriptor */
        0x12, 0x01, 0x10, 0x01, 0x00, 0x00, 0x00, 0x08,
        0xec, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01,                                      

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32, 

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
        0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x01, 0x02, 0x40, 0x00, 0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x82, 0x02, 0x40, 0x00, 0x00 
};
    
#define DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED 60
static UCHAR device_framework_high_speed[] = { 
    /* Device descriptor */
        0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x0a, 0x07, 0x25, 0x40, 0x01, 0x00, 0x01, 0x02,
        0x03, 0x01,                                      

    /* Device qualifier descriptor */
        0x0a, 0x06, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
        0x01, 0x00,

    /* Configuration descriptor */
        0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0xc0,
        0x32, 

    /* Interface descriptor */
        0x09, 0x04, 0x00, 0x00, 0x02, 0x99, 0x99, 0x99,
        0x00,

    /* Endpoint descriptor (Bulk Out) */
        0x07, 0x05, 0x01, 0x02, 0x00, 0x02, 0x00,

    /* Endpoint descriptor (Bulk In) */
        0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00 
};
    
/* String Device Framework :
    Byte 0 and 1 : Word containing the language ID : 0x0904 for US
    Byte 2       : Byte containing the index of the descriptor
    Byte 3       : Byte containing the length of the descriptor string
*/
#define STRING_FRAMEWORK_LENGTH 38
static UCHAR string_framework[] = { 
    /* Manufacturer string descriptor : Index 1 */
        0x09, 0x04, 0x01, 0x0c, 
        0x45, 0x78, 0x70, 0x72,0x65, 0x73, 0x20, 0x4c, 
        0x6f, 0x67, 0x69, 0x63,

    /* Product string descriptor : Index 2 */
        0x09, 0x04, 0x02, 0x0c, 
        0x44, 0x61, 0x74, 0x61, 0x50, 0x75, 0x6d, 0x70, 
        0x44, 0x65, 0x6d, 0x6f,  

    /* Serial Number string descriptor : Index 3 */
        0x09, 0x04, 0x03, 0x04, 
        0x30, 0x30, 0x30, 0x31
};

/* Multiple languages are supported on the device, to add
    a language besides English, the unicode language code must
    be appended to the language_id_framework array and the length
    adjusted accordingly. */
#define LANGUAGE_ID_FRAMEWORK_LENGTH 2
static UCHAR language_id_framework[] = { 
    /* English. */
    0x09, 0x04
};


static void _ux_dcd_stm32_interrupt_thread(void *arg) {
	(void) arg;

	for ( ; ; ) {
		tx_semaphore_get(&dcd_irq_signal, TX_WAIT_FOREVER);

		/* Process host controller interrupt */
		_ux_dcd_stm32_interrupt_handler();
	}
}

static void __fastcode _ux_dcd_stm32_interrupt_routine(void *arg) {
	PCD_HandleTypeDef *pcd = &usb_device_handle;
	PCD_TypeDef *reg = pcd->Instance;
	uint32_t sta = reg->GINTSTS;

	/* Clear pending interrupt flags */
	reg->GINTSTS = sta;
	pcd->Pending = sta & reg->GINTMSK;

	/* If does have any interrupt pending then wake up interrupt thread */
	if (pcd->Pending)
		tx_semaphore_ceiling_put(&dcd_irq_signal, 1);
}

static int stm32_usbdevice_irq_initialize(void) {
	const int nr_irqs[] = {
		OTG_FS_EP1_OUT_IRQn,
		OTG_FS_EP1_IN_IRQn,
		OTG_FS_WKUP_IRQn,
		OTG_FS_IRQn
	};

	/* Create interrupt sync semaphore */
	tx_semaphore_create(&dcd_irq_signal, "dcd_signal", 0);

	/* Create interrupt service thread */
	tx_thread_spawn(&dcd_irq_thread, "hcd_irq", _ux_dcd_stm32_interrupt_thread, 
		0, dcd_irq_stack, sizeof(dcd_irq_stack), 8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);
	
	/* Install hardware interrupt process handler */
	for (size_t i = 0; i < rte_array_size(nr_irqs); i++) {
		int err = request_irq(nr_irqs[i], _ux_dcd_stm32_interrupt_routine, NULL);
		if (err)
			return err;
	}

	return 0;
}

static int stm32_usbdevice_init(void) {
	static ULONG _ux_memory[UX_MEMORY_SIZE / sizeof(ULONG)];
	UINT err;

	HAL_PCD_Init(&usb_device_handle);

	/* Install hardware interrupt service for USB host controller */
	err = (UINT)stm32_usbdevice_irq_initialize();
	if (err)
		return err;	

	/* Initialize USBX Memory.  */
	err = _ux_system_initialize(_ux_memory, sizeof(_ux_memory), UX_NULL, 0);
	if (err != UX_SUCCESS)
		return _UX_ERR(err);

    /* Initialize USB device stack */
    err = _ux_device_stack_initialize(
            device_framework_high_speed, DEVICE_FRAMEWORK_LENGTH_HIGH_SPEED,
            device_framework_full_speed, DEVICE_FRAMEWORK_LENGTH_FULL_SPEED,
            string_framework, STRING_FRAMEWORK_LENGTH,
            language_id_framework, LANGUAGE_ID_FRAMEWORK_LENGTH, UX_NULL);
    if (err)
        return _UX_ERR(err);

    err = _ux_dcd_stm32_initialize(0, (ULONG)&usb_device_handle);
    if (err)
        return _UX_ERR(err);

    /* Register USB storage class */
    // err = ux_device_stack_class_register(_ux_system_slave_class_storage_name,
    //     ux_device_class_storage_entry, 1, 0, &class_storage);

    return 0;
}

SYSINIT(stm32_usbdevice_init, SI_BUSDRIVER_LEVEL, 60);
