/*
 * Copyright 2024 wtcat
 */
#define UX_SOURCE_CODE


#include "ux_api.h"
#include "ux_host_stack.h"

#include "stm32h7xx_hal_pcd.h"




/* Define STM32 HCD generic definitions.  */


#define UX_HCD_STM32_CONTROLLER                                 6U
#ifndef UX_HCD_STM32_MAX_NB_CHANNELS
#define UX_HCD_STM32_MAX_NB_CHANNELS                            12U
#endif /* UX_HCD_STM32_MAX_NB_CHANNELS */

#define UX_HCD_STM32_MAX_HUB_BINTERVAL                          8U
#define UX_HCD_STM32_NB_ROOT_PORTS                              1U
#define UX_HCD_STM32_NO_CHANNEL_ASSIGNED                        0xffU
#define UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED            0x01U
#define UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_DETACHED            0x02U
#define UX_HCD_STM32_CONTROLLER_FLAG_SOF                        0x04U
#define UX_HCD_STM32_CONTROLLER_FLAG_TRANSFER_DONE              0x08U
#define UX_HCD_STM32_CONTROLLER_FLAG_TRANSFER_ERROR             0x10U
#define UX_HCD_STM32_CONTROLLER_LOW_SPEED_DEVICE                0x20U
#define UX_HCD_STM32_CONTROLLER_FULL_SPEED_DEVICE               0x40U
#define UX_HCD_STM32_CONTROLLER_HIGH_SPEED_DEVICE               0x80U
#define UX_HCD_STM32_MAX_PACKET_COUNT                           256U

#define UX_HCD_STM32_ED_STATUS_FREE                             0x00U
#define UX_HCD_STM32_ED_STATUS_ALLOCATED                        0x01U
#define UX_HCD_STM32_ED_STATUS_ABORTED                          0x02U
#define UX_HCD_STM32_ED_STATUS_CONTROL_SETUP                    0x03U
#define UX_HCD_STM32_ED_STATUS_CONTROL_DATA_IN                  0x04U
#define UX_HCD_STM32_ED_STATUS_CONTROL_DATA_OUT                 0x05U
#define UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_IN                0x06U
#define UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_OUT               0x07U
#define UX_HCD_STM32_ED_STATUS_BULK_IN                          0x08U
#define UX_HCD_STM32_ED_STATUS_BULK_OUT                         0x09U
#define UX_HCD_STM32_ED_STATUS_PERIODIC_TRANSFER                0x0AU
#define UX_HCD_STM32_ED_STATUS_PENDING_MASK                     0x0FU
#define UX_HCD_STM32_ED_STATUS_TRANSFER_DONE                    0x10U


/* Define STM32 static definition.  */

#define UX_HCD_STM32_AVAILABLE_BANDWIDTH                        6000U


/* Define STM32 structure.  */

typedef struct UX_HCD_STM32_STRUCT
{

    struct UX_HCD_STRUCT                *ux_hcd_stm32_hcd_owner;
    struct UX_HCD_STM32_ED_STRUCT       *ux_hcd_stm32_ed_list;
    struct UX_HCD_STM32_ED_STRUCT       *ux_hcd_stm32_channels_ed[UX_HCD_STM32_MAX_NB_CHANNELS];
    ULONG                               ux_hcd_stm32_nb_channels;
    UINT                                ux_hcd_stm32_queue_empty;
    UINT                                ux_hcd_stm32_periodic_scheduler_active;
    ULONG                               ux_hcd_stm32_controller_flag;
    HCD_HandleTypeDef                   *hcd_handle;
    struct UX_HCD_STM32_ED_STRUCT       *ux_hcd_stm32_periodic_ed_head;
} UX_HCD_STM32;


/* Define STM32 ED structure.  */

typedef struct UX_HCD_STM32_ED_STRUCT
{

    struct UX_HCD_STM32_ED_STRUCT       *ux_stm32_ed_next_ed;
    struct UX_ENDPOINT_STRUCT           *ux_stm32_ed_endpoint;
    struct UX_TRANSFER_STRUCT           *ux_stm32_ed_transfer_request;
    UCHAR                               *ux_stm32_ed_setup;
    UCHAR                               *ux_stm32_ed_data;
    USHORT                              ux_stm32_ed_saved_length;
    USHORT                              ux_stm32_ed_saved_actual_length;
    ULONG                               ux_stm32_ed_packet_length;
    ULONG                               ux_stm32_ed_interval_mask;
    ULONG                               ux_stm32_ed_interval_position;
    ULONG                               ux_stm32_ed_current_ss_frame;
    UCHAR                               ux_stm32_ed_status;
    UCHAR                               ux_stm32_ed_channel;
    UCHAR                               ux_stm32_ed_dir;
    UCHAR                               ux_stm32_ed_speed;
    UCHAR                               ux_stm32_ed_type;
    UCHAR                               ux_stm32_ed_sch_mode;
    UCHAR                               reserved[2];
} UX_HCD_STM32_ED;


#define USBH_PID_SETUP                            0U
#define USBH_PID_DATA                             1U


void HAL_HCD_Connect_Callback(HCD_HandleTypeDef *hhcd)
{

UX_HCD              *hcd;
UX_HCD_STM32        *hcd_stm32;


    /* Get the pointer to the HCD & HCD_STM32.  */
    hcd = (UX_HCD*)hhcd -> pData;
    hcd_stm32 = (UX_HCD_STM32*)hcd -> ux_hcd_controller_hardware;

    /* Something happened on the root hub port. Signal it to the root hub     thread.  */
    hcd -> ux_hcd_root_hub_signal[0]++;

    /* The controller has issued a ATTACH Root HUB signal.  */
    hcd_stm32 -> ux_hcd_stm32_controller_flag |= UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED;
    hcd_stm32 -> ux_hcd_stm32_controller_flag &= ~UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_DETACHED;

    /* Wake up the root hub thread.  */
    _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
}

void HAL_HCD_Disconnect_Callback(HCD_HandleTypeDef *hhcd)
{

UX_HCD              *hcd;
UX_HCD_STM32        *hcd_stm32;


    /* Get the pointer to the HCD & HCD_STM32.  */
    hcd = (UX_HCD*)hhcd -> pData;
    hcd_stm32 = (UX_HCD_STM32*)hcd -> ux_hcd_controller_hardware;

    /* Something happened on the root hub port. Signal it to the root hub     thread.  */
    hcd -> ux_hcd_root_hub_signal[0]++;

    /* The controller has issued a DETACH Root HUB signal.  */
    hcd_stm32 -> ux_hcd_stm32_controller_flag |= UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_DETACHED;
    hcd_stm32 -> ux_hcd_stm32_controller_flag &= ~UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED;

    /* Wake up the root hub thread.  */
    _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_enum_semaphore);
}

void HAL_HCD_HC_NotifyURBChange_Callback(HCD_HandleTypeDef *hhcd, uint8_t chnum, HCD_URBStateTypeDef urb_state)
{

UX_HCD              *hcd;
UX_HCD_STM32        *hcd_stm32;
UX_HCD_STM32_ED     *ed;
UX_TRANSFER         *transfer_request;
UX_TRANSFER         *transfer_next;


    /* Check the URB state.  */
    if (urb_state == URB_DONE || urb_state == URB_STALL || urb_state == URB_ERROR || urb_state == URB_NOTREADY)
    {

        /* Get the pointer to the HCD & HCD_STM32.  */
        hcd = (UX_HCD*)hhcd -> pData;
        hcd_stm32 = (UX_HCD_STM32*)hcd -> ux_hcd_controller_hardware;

        /* Check if driver is still valid.  */
        if (hcd_stm32 == UX_NULL)
            return;

        /* Load the ED for the channel.  */
        ed =  hcd_stm32 -> ux_hcd_stm32_channels_ed[chnum];

        /* Check if ED is still valid.  */
        if (ed == UX_NULL)
        {
            return;
        }

        /* Get transfer request.  */
        transfer_request = ed -> ux_stm32_ed_transfer_request;

        /* Check if request is still valid.  */
        if (transfer_request == UX_NULL)
        {
            return;
        }

        /* Check if URB state is not URB_NOTREADY.  */
        if (urb_state != URB_NOTREADY)
        {

            /* Handle URB states.  */
            switch (urb_state)
            {
            case URB_STALL:

                /* Set the completion code to stalled.  */
                transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_STALLED;
                break;

            case URB_DONE:

                /* Check the request direction.  */
                if (ed -> ux_stm32_ed_dir == 1)
                {
                  if ((ed -> ux_stm32_ed_type == EP_TYPE_CTRL) || (ed -> ux_stm32_ed_type == EP_TYPE_BULK))
                  {
                    /* Get transfer size for receiving direction. */
                    transfer_request -> ux_transfer_request_actual_length += HAL_HCD_HC_GetXferCount(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel);

                    /* Check if there is more data to be received. */
                    if ((transfer_request -> ux_transfer_request_requested_length > transfer_request -> ux_transfer_request_actual_length) &&
                       (HAL_HCD_HC_GetXferCount(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel) == ed->ux_stm32_ed_endpoint->ux_endpoint_descriptor.wMaxPacketSize))
                    {
                      /* Adjust the transmit length.  */
                      ed -> ux_stm32_ed_packet_length = UX_MIN(ed->ux_stm32_ed_endpoint->ux_endpoint_descriptor.wMaxPacketSize,
                                                               transfer_request -> ux_transfer_request_requested_length - transfer_request -> ux_transfer_request_actual_length);

                      /* Submit the transmit request.  */
                      HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                               1U,
                                               ed -> ux_stm32_ed_type,
                                               USBH_PID_DATA,
                                               ed -> ux_stm32_ed_data + transfer_request -> ux_transfer_request_actual_length,
                                               ed -> ux_stm32_ed_packet_length, 0);
                      return;
                    }
                  }
                  else
                  {
                    /* Get transfer size for receiving direction. */
                    transfer_request -> ux_transfer_request_actual_length = HAL_HCD_HC_GetXferCount(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel);
                  }
                }

                /* Check if the request is for OUT transfer.  */
                if (ed -> ux_stm32_ed_dir == 0U)
                {

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
                  if ((hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].do_ssplit == 1U) && (ed -> ux_stm32_ed_type == EP_TYPE_ISOC) &&
                      (ed -> ux_stm32_ed_packet_length > hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].max_packet))
                  {
                    /* Update actual transfer length with ISOC max packet size for split transaction  */
                    transfer_request -> ux_transfer_request_actual_length += hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].max_packet;
                  }
                  else
#endif /*defined (USBH_HAL_HUB_SPLIT_SUPPORTED) */
                  {
                    /* Update actual transfer length.  */
                    transfer_request -> ux_transfer_request_actual_length += ed -> ux_stm32_ed_packet_length;
                  }

                    /* Check if there is more data to send.  */
                    if (transfer_request -> ux_transfer_request_requested_length >
                        transfer_request -> ux_transfer_request_actual_length)
                    {

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
                      if ((hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].do_ssplit == 1U) && (ed -> ux_stm32_ed_type == EP_TYPE_ISOC) &&
                          (ed -> ux_stm32_ed_packet_length > hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].max_packet))
                      {
                        /* Adjust the transmit length.  */
                        ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_packet_length - \
                                                          hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].max_packet;

                        /* Submit the transmit request.  */
                        HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                                 0, ed -> ux_stm32_ed_type, USBH_PID_DATA,
                                                 ed -> ux_stm32_ed_data + transfer_request -> ux_transfer_request_actual_length,
                                                 ed -> ux_stm32_ed_packet_length, 0);
                        return;
                      }
#endif /* defined (USBH_HAL_HUB_SPLIT_SUPPORTED) */

                        /* Periodic transfer that needs schedule is not started here.  */
                        if (ed -> ux_stm32_ed_sch_mode)
                            return;

                        /* Adjust the transmit length.  */
                        ed -> ux_stm32_ed_packet_length =
                            UX_MIN(transfer_request -> ux_transfer_request_packet_length,
                                   transfer_request -> ux_transfer_request_requested_length -
                                   transfer_request -> ux_transfer_request_actual_length);

                        /* Submit the transmit request.  */
                        HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                                 0, ed -> ux_stm32_ed_type, USBH_PID_DATA,
                                                 ed -> ux_stm32_ed_data + transfer_request -> ux_transfer_request_actual_length,
                                                 ed -> ux_stm32_ed_packet_length, 0);
                        return;
                    }
                }

                /* Set the completion code to SUCCESS.  */
                transfer_request -> ux_transfer_request_completion_code =  UX_SUCCESS;
                break;

            default:
                /* Set the completion code to transfer error.  */
                transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_ERROR;
            }

            /* Finish current transfer.  */
            _ux_hcd_stm32_request_trans_finish(hcd_stm32, ed);

            /* Move to next transfer.  */
            transfer_next = transfer_request -> ux_transfer_request_next_transfer_request;
            ed -> ux_stm32_ed_transfer_request = transfer_next;

            /* If there is transfer to start, start it.  */
            if (transfer_next)
            {

                /* If transfer is not started by schedular, start here.  */
                if (!ed -> ux_stm32_ed_sch_mode)
                {

                    /* For ISO OUT, packet size is from request variable,
                    * otherwise, use request length.  */
                    if ((ed -> ux_stm32_ed_type == EP_TYPE_ISOC) && (ed -> ux_stm32_ed_dir == 0))
                        ed -> ux_stm32_ed_packet_length = transfer_next -> ux_transfer_request_packet_length;
                    else
                        ed -> ux_stm32_ed_packet_length = transfer_next -> ux_transfer_request_requested_length;

                    /* Prepare transactions.  */
                    _ux_hcd_stm32_request_trans_prepare(hcd_stm32, ed, transfer_next);

                    /* Call HAL driver to submit the transfer request.  */
                    HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                             ed -> ux_stm32_ed_dir,
                                             ed -> ux_stm32_ed_type, USBH_PID_DATA,
                                             ed -> ux_stm32_ed_data + transfer_next -> ux_transfer_request_actual_length,
                                             ed -> ux_stm32_ed_packet_length, 0);
                }
            }
            else
            {

                /* Transfer not continued, periodic needs re-schedule.  */
                if ((ed -> ux_stm32_ed_type == EP_TYPE_INTR) ||
                    (ed -> ux_stm32_ed_type == EP_TYPE_ISOC))
                    ed -> ux_stm32_ed_sch_mode = 1;
                }

#if defined(UX_HOST_STANDALONE)
            transfer_request -> ux_transfer_request_status = UX_TRANSFER_STATUS_COMPLETED;
            ed -> ux_stm32_ed_status |= UX_HCD_STM32_ED_STATUS_TRANSFER_DONE;
#endif /* defined(UX_HOST_STANDALONE) */

            /* Invoke callback function.  */
            if (transfer_request -> ux_transfer_request_completion_function)
                transfer_request -> ux_transfer_request_completion_function(transfer_request);

            /* Wake up the transfer request thread.  */
            _ux_host_semaphore_put(&transfer_request -> ux_transfer_request_semaphore);

        }
        else
        {

            /* Handle URB_NOTREADY state here.  */
            /* Check if we need to retry the transfer by checking the status.  */
            if ((ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_CONTROL_SETUP) ||
                (ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_CONTROL_DATA_OUT) ||
                (ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_OUT) ||
                (ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_BULK_OUT))
            {

                /* Submit the transmit request.  */
                HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel, 0,
                                        ((ed -> ux_stm32_ed_endpoint -> ux_endpoint_descriptor.bmAttributes) & UX_MASK_ENDPOINT_TYPE) == UX_BULK_ENDPOINT ? EP_TYPE_BULK : EP_TYPE_CTRL,
                                         ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_CONTROL_SETUP ? USBH_PID_SETUP : USBH_PID_DATA,
                                         ed -> ux_stm32_ed_data + transfer_request -> ux_transfer_request_actual_length,
                                         ed -> ux_stm32_ed_packet_length, 0);
            }

        }
    }
}

void HAL_HCD_SOF_Callback(HCD_HandleTypeDef *hhcd)
{

UX_HCD              *hcd;
UX_HCD_STM32        *hcd_stm32;


    /* Get the pointer to the HCD & HCD_STM32.  */
    hcd = (UX_HCD*)hhcd -> pData;
    hcd_stm32 = (UX_HCD_STM32*)hcd -> ux_hcd_controller_hardware;

    if ((hcd_stm32 -> ux_hcd_stm32_controller_flag & UX_HCD_STM32_CONTROLLER_FLAG_SOF) == 0)
    {
        hcd_stm32 -> ux_hcd_stm32_controller_flag |= UX_HCD_STM32_CONTROLLER_FLAG_SOF;
        hcd -> ux_hcd_thread_signal++;

        /* Wake up the scheduler.  */
        _ux_host_semaphore_put(&_ux_system_host -> ux_system_host_hcd_semaphore);
    }
}

UINT  _ux_hcd_stm32_controller_disable(UX_HCD_STM32 *hcd_stm32)
{

UX_HCD      *hcd;

    /* Point to the generic portion of the host controller structure instance.  */
    hcd =  hcd_stm32 -> ux_hcd_stm32_hcd_owner;

    /* Reflect the state of the controller in the main structure.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_HALTED;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UX_HCD_STM32_ED  *_ux_hcd_stm32_ed_obtain(UX_HCD_STM32 *hcd_stm32)
{

UX_HCD_STM32_ED       *ed;
ULONG                 ed_index;


    /* Start the search from the beginning of the list.  */
    ed =  hcd_stm32 -> ux_hcd_stm32_ed_list;
    for (ed_index = 0; ed_index < _ux_system_host -> ux_system_host_max_ed; ed_index++)
    {

        /* Check the ED status, a free ED is marked with the UNUSED flag.  */
        if (ed -> ux_stm32_ed_status == UX_HCD_STM32_ED_STATUS_FREE)
        {

            /* The ED may have been used, so we reset all fields.  */
            _ux_utility_memory_set(ed, 0, sizeof(UX_HCD_STM32_ED));

            /* This ED is now marked as ALLOCATED.  */
            ed -> ux_stm32_ed_status =  UX_HCD_STM32_ED_STATUS_ALLOCATED;

            /* Reset the channel.  */
            ed -> ux_stm32_ed_channel =  UX_HCD_STM32_NO_CHANNEL_ASSIGNED;

            /* Return ED pointer.  */
            return(ed);
        }

        /* Point to the next ED.  */
        ed++;
    }

    /* There is no available ED in the ED list.  */
    return(UX_NULL);
}

UINT  _ux_hcd_stm32_endpoint_create(UX_HCD_STM32 *hcd_stm32, UX_ENDPOINT *endpoint)
{

UX_HCD_STM32_ED        *ed;
UX_DEVICE              *device;
ULONG                   channel_index;
UINT                    device_speed;
UINT                    endpoint_type;
ULONG                   packet_size;
ULONG                   endpoint_bInterval;


    /* Get the pointer to the device.  */
    device =  endpoint -> ux_endpoint_device;

    /* Set device speed.  */
    switch (device -> ux_device_speed)
    {
    case UX_HIGH_SPEED_DEVICE:
        device_speed =  HCD_DEVICE_SPEED_HIGH;
        break;
    case UX_FULL_SPEED_DEVICE:
        device_speed =  HCD_DEVICE_SPEED_FULL;
        break;
    case UX_LOW_SPEED_DEVICE:
        device_speed =  HCD_DEVICE_SPEED_LOW;
        break;
    default:
        return(UX_ERROR);
    }

    /* Set endpoint type.  */
    switch ((endpoint -> ux_endpoint_descriptor.bmAttributes) & UX_MASK_ENDPOINT_TYPE)
    {
    case UX_CONTROL_ENDPOINT:
        endpoint_type =  EP_TYPE_CTRL;
        break;
    case UX_BULK_ENDPOINT:
        endpoint_type =  EP_TYPE_BULK;
        break;
    case UX_INTERRUPT_ENDPOINT:
        endpoint_type =  EP_TYPE_INTR;
       break;
    case UX_ISOCHRONOUS_ENDPOINT:
        endpoint_type =  EP_TYPE_ISOC;
        break;
    default:
        return(UX_FUNCTION_NOT_SUPPORTED);
    }

    /* Obtain a ED for this new endpoint. This ED will live as long as the endpoint is active
       and will be the container for the tds.  */
    ed =  _ux_hcd_stm32_ed_obtain(hcd_stm32);
    if (ed == UX_NULL)
        return(UX_NO_ED_AVAILABLE);

    /* And get a channel. */
    for (channel_index = 0; channel_index < hcd_stm32 -> ux_hcd_stm32_nb_channels; channel_index++)
    {

        /* Check if that Channel is free.  */
        if (hcd_stm32 -> ux_hcd_stm32_channels_ed[channel_index]  == UX_NULL)
        {

            /* We have a channel. Save it. */
            hcd_stm32 -> ux_hcd_stm32_channels_ed[channel_index] = ed;

            /* And in the endpoint too. */
            ed -> ux_stm32_ed_channel = channel_index;

            /* Done here.  */
            break;

        }
    }

    /* Check for channel assignment.  */
    if (ed -> ux_stm32_ed_channel ==  UX_HCD_STM32_NO_CHANNEL_ASSIGNED)
    {

        /* Free the ED.  */
        ed -> ux_stm32_ed_status =  UX_HCD_STM32_ED_STATUS_FREE;

        /* Could not allocate a channel.  */
        return(UX_NO_ED_AVAILABLE);
    }

    /* Check for interrupt and isochronous endpoints.  */
    if ((endpoint_type == EP_TYPE_INTR) || (endpoint_type == EP_TYPE_ISOC))
    {
      if (device_speed == HCD_DEVICE_SPEED_HIGH)
      {
        if ((device->ux_device_current_configuration->ux_configuration_first_interface->ux_interface_descriptor.bInterfaceClass == 0x9U) &&
            (endpoint -> ux_endpoint_descriptor.bInterval > 9U))
        {
          /* Some hubs has an issue with larger binterval scheduling */
            endpoint_bInterval = UX_HCD_STM32_MAX_HUB_BINTERVAL;
        }
        else
        {
          endpoint_bInterval = endpoint -> ux_endpoint_descriptor.bInterval;
        }

        /* Set the interval mask for high speed or isochronous endpoints.  */
        ed -> ux_stm32_ed_interval_mask = (1 << (endpoint_bInterval - 1U)) - 1U;
      }
      else
      {

        /* Set the interval mask for other endpoints.  */
        ed -> ux_stm32_ed_interval_mask = endpoint -> ux_endpoint_descriptor.bInterval;

#if UX_MAX_DEVICES > 1
        if (device->ux_device_parent != NULL)
        {
          if (device->ux_device_parent->ux_device_speed == UX_HIGH_SPEED_DEVICE)
          {
            ed -> ux_stm32_ed_interval_mask <<= 3U;
          }
        }
#endif

        ed -> ux_stm32_ed_interval_mask |= ed -> ux_stm32_ed_interval_mask >> 1;
        ed -> ux_stm32_ed_interval_mask |= ed -> ux_stm32_ed_interval_mask >> 2;
        ed -> ux_stm32_ed_interval_mask |= ed -> ux_stm32_ed_interval_mask >> 4;
        ed -> ux_stm32_ed_interval_mask >>= 1;
      }

        /* Select a transfer time slot with least traffic.  */
        if (ed -> ux_stm32_ed_interval_mask == 0)
            ed -> ux_stm32_ed_interval_position = 0;
        else
        ed -> ux_stm32_ed_interval_position =  _ux_hcd_stm32_least_traffic_list_get(hcd_stm32);

        /* No transfer on going.  */
        ed -> ux_stm32_ed_transfer_request = UX_NULL;

        /* Attach the ed to periodic ed list.  */
        ed -> ux_stm32_ed_next_ed = hcd_stm32 -> ux_hcd_stm32_periodic_ed_head;
        hcd_stm32 -> ux_hcd_stm32_periodic_ed_head = ed;

        /* Activate periodic scheduler.  */
        hcd_stm32 -> ux_hcd_stm32_periodic_scheduler_active ++;
    }

    /* Attach the ED to the endpoint container.  */
    endpoint -> ux_endpoint_ed =  (VOID *) ed;

    /* Now do the opposite, attach the ED container to the physical ED.  */
    ed -> ux_stm32_ed_endpoint =  endpoint;
    ed -> ux_stm32_ed_speed = (UCHAR)device_speed;
    ed -> ux_stm32_ed_dir = (endpoint -> ux_endpoint_descriptor.bEndpointAddress & 0x80) ? 1 : 0;
    ed -> ux_stm32_ed_type     =  endpoint_type;
    packet_size                =  endpoint -> ux_endpoint_descriptor.wMaxPacketSize & UX_MAX_PACKET_SIZE_MASK;
    if (endpoint -> ux_endpoint_descriptor.wMaxPacketSize & UX_MAX_NUMBER_OF_TRANSACTIONS_MASK)
    {

        /* Free the ED.  */
        ed -> ux_stm32_ed_status =  UX_HCD_STM32_ED_STATUS_FREE;

        /* High bandwidth are not supported for now.  */
        return(UX_FUNCTION_NOT_SUPPORTED);
    }

    /* By default scheduler is not needed.  */
    ed -> ux_stm32_ed_sch_mode = 0;

    /* By default data pointer is not used.  */
    ed -> ux_stm32_ed_data = UX_NULL;

    /* Call HAL to initialize the host channel.  */
    HAL_HCD_HC_Init(hcd_stm32->hcd_handle,
                    channel_index,
                    endpoint -> ux_endpoint_descriptor.bEndpointAddress,
                    device -> ux_device_address,
                    device_speed,
                    endpoint_type,
                    endpoint -> ux_endpoint_descriptor.wMaxPacketSize);

    /* Reset toggles.  */
    hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].toggle_in = 0;
    hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].toggle_out = 0;

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
    /* Check if device connected to hub  */
    if (endpoint->ux_endpoint_device->ux_device_parent != NULL)
    {
      HAL_HCD_HC_SetHubInfo(hcd_stm32->hcd_handle, ed->ux_stm32_ed_channel,
                            endpoint->ux_endpoint_device->ux_device_parent->ux_device_address,
                            endpoint->ux_endpoint_device->ux_device_port_location);
    }
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */

    /* We need to take into account the nature of the HCD to define the max size
       of any transfer in the transfer request.  */
    endpoint -> ux_endpoint_transfer_request.ux_transfer_request_maximum_length =  UX_HCD_STM32_MAX_PACKET_COUNT * packet_size;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_endpoint_destroy(UX_HCD_STM32 *hcd_stm32, UX_ENDPOINT *endpoint)
{

#if defined(UX_HOST_STANDALONE)
UX_INTERRUPT_SAVE_AREA
#endif /* defined(UX_HOST_STANDALONE) */
UX_HCD_STM32_ED       *ed;
UX_HCD_STM32_ED       *next_ed;
UINT                   endpoint_type;

    /* From the endpoint container fetch the STM32 ED descriptor.  */
    ed =  (UX_HCD_STM32_ED *) endpoint -> ux_endpoint_ed;

    /* Check if this physical endpoint has been initialized properly!  */
    if (ed == UX_NULL)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_ENDPOINT_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, endpoint, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);

    }

#if defined(UX_HOST_STANDALONE)

    /* There is no background thread, just remove the ED from processing list.  */
    UX_DISABLE
#else

    /* Wait for the controller to finish the current frame processing.  */
    _ux_utility_delay_ms(1);
#endif /* defined(UX_HOST_STANDALONE) */

    /* We need to free the channel.  */
    hcd_stm32 -> ux_hcd_stm32_channels_ed[ed -> ux_stm32_ed_channel] =  UX_NULL;

    /* Get endpoint type.  */
    endpoint_type = (endpoint -> ux_endpoint_descriptor.bmAttributes) & UX_MASK_ENDPOINT_TYPE;

    /* Check for periodic endpoints.  */
    if ((endpoint_type == UX_INTERRUPT_ENDPOINT) || (endpoint_type == UX_ISOCHRONOUS_ENDPOINT))
    {

        /* Remove the ED from periodic ED list.  */
        if (hcd_stm32 -> ux_hcd_stm32_periodic_ed_head == ed)
        {

            /* The head one in the list, just set the pointer to it's next.  */
            hcd_stm32 -> ux_hcd_stm32_periodic_ed_head = ed -> ux_stm32_ed_next_ed;
        }
        else
        {

            /* Get the first ED in the list.  */
            next_ed = hcd_stm32 -> ux_hcd_stm32_periodic_ed_head;

            /* Search for the ED in the list.  */
            while( (next_ed != UX_NULL) && (next_ed -> ux_stm32_ed_next_ed != ed) )
            {

                /* Move to next ED.  */
                next_ed = next_ed -> ux_stm32_ed_next_ed;
            }

            /* Check if we found the ED.  */
            if (next_ed)
            {

                /* Remove the ED from list.  */
                next_ed -> ux_stm32_ed_next_ed = next_ed -> ux_stm32_ed_next_ed -> ux_stm32_ed_next_ed;
            }
        }

        /* Decrease the periodic active count.  */
        hcd_stm32 -> ux_hcd_stm32_periodic_scheduler_active --;
    }

    /* Now we can safely make the ED free.  */
    ed -> ux_stm32_ed_status =  UX_HCD_STM32_ED_STATUS_FREE;

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
    HAL_HCD_HC_ClearHubInfo(hcd_stm32->hcd_handle, ed -> ux_stm32_ed_channel);
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */

    /* Finish current transfer.  */
    _ux_hcd_stm32_request_trans_finish(hcd_stm32, ed);

#if defined(UX_HOST_STANDALONE)

    /* If setup memory is not freed correct, free it.  */
    if (ed -> ux_stm32_ed_setup)
        _ux_utility_memory_free(ed -> ux_stm32_ed_setup);

    UX_RESTORE
#endif /* defined(UX_HOST_STANDALONE) */

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_endpoint_reset(UX_HCD_STM32 *hcd_stm32, UX_ENDPOINT *endpoint)
{

UX_HCD_STM32_ED       *ed;


    /* From the endpoint container fetch the STM32 ED descriptor.  */
    ed =  (UX_HCD_STM32_ED *) endpoint -> ux_endpoint_ed;

    /* Finish current transfer.  */
    _ux_hcd_stm32_request_trans_finish(hcd_stm32, ed);

    /* Reset the data0/data1 toggle bit.  */
    hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].toggle_in = 0;
    hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].toggle_out = 0;

    /* This operation never fails.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_frame_number_get(UX_HCD_STM32 *hcd_stm32, ULONG *frame_number)
{

    /* Call HAL to get the frame number.  */
    *frame_number = (ULONG)HAL_HCD_GetCurrentFrame(hcd_stm32 -> hcd_handle);

    return(UX_SUCCESS);
}

VOID  _ux_hcd_stm32_interrupt_handler(VOID)
{

UINT                hcd_index;
UX_HCD              *hcd;
UX_HCD_STM32        *hcd_stm32;


    /* We need to parse the controller driver table to find all controllers that
      are registered as STM32.  */
    for (hcd_index = 0; hcd_index < _ux_system_host -> ux_system_host_registered_hcd; hcd_index++)
    {

        /* Check type of controller.  */
        if (_ux_system_host -> ux_system_host_hcd_array[hcd_index].ux_hcd_controller_type == UX_HCD_STM32_CONTROLLER)
        {

            /* Get the pointers to the generic HCD and STM32 specific areas.  */
            hcd =  &_ux_system_host -> ux_system_host_hcd_array[hcd_index];
            hcd_stm32 =  (UX_HCD_STM32 *) hcd -> ux_hcd_controller_hardware;

            /* Call HAL interrupt handler.  */
            HAL_HCD_IRQHandler(hcd_stm32 -> hcd_handle);
        }
    }
}

UINT  _ux_hcd_stm32_least_traffic_list_get(UX_HCD_STM32 *hcd_stm32)
{

UX_HCD_STM32_ED     *ed;
UINT                list_index;
ULONG               min_bandwidth_used;
ULONG               bandwidth_used;
UINT                min_bandwidth_slot;


    /* Set the min bandwidth used to a arbitrary maximum value.  */
    min_bandwidth_used =  0xffffffff;

    /* The first ED is the list candidate for now.  */
    min_bandwidth_slot =  0;

    /* All list will be scanned.  */
    for (list_index = 0; list_index < 32; list_index++)
    {

        /* Reset the bandwidth for this list.  */
        bandwidth_used =  0;

        /* Get the ED of the beginning of the list we parse now.  */
        ed =  hcd_stm32 -> ux_hcd_stm32_periodic_ed_head;

        /* Parse the eds in the list.  */
        while (ed != UX_NULL)
        {

            if ((list_index & ed -> ux_stm32_ed_interval_mask) == ed -> ux_stm32_ed_interval_position)
            {

                /* Add to the bandwidth used the max packet size pointed by this ED.  */
                bandwidth_used +=  (ULONG) ed -> ux_stm32_ed_endpoint -> ux_endpoint_descriptor.wMaxPacketSize;
            }

            /* Move to next ED.  */
            ed =  ed -> ux_stm32_ed_next_ed;
        }

        /* We have processed a list, check the bandwidth used by this list.
           If this bandwidth is the minimum, we memorize the ED.  */
        if (bandwidth_used < min_bandwidth_used)
        {

            /* We have found a better list with a lower used bandwidth, memorize the bandwidth
               for this list.  */
            min_bandwidth_used =  bandwidth_used;

            /* Memorize the begin ED for this list.  */
            min_bandwidth_slot =  list_index;
        }
    }

    /* Return the ED list with the lowest bandwidth.  */
    return(min_bandwidth_slot);
}

UINT  _ux_hcd_stm32_initialize(UX_HCD *hcd)
{

UX_HCD_STM32          *hcd_stm32;


    /* The controller initialized here is of STM32 type.  */
    hcd -> ux_hcd_controller_type =  UX_HCD_STM32_CONTROLLER;

    /* Initialize the max bandwidth for periodic endpoints. On STM32, the spec says
       no more than 90% to be allocated for periodic.  */
#if UX_MAX_DEVICES > 1
    hcd -> ux_hcd_available_bandwidth =  UX_HCD_STM32_AVAILABLE_BANDWIDTH;
#endif

    /* Allocate memory for this STM32 HCD instance.  */
    hcd_stm32 =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HCD_STM32));
    if (hcd_stm32 == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Set the pointer to the STM32 HCD.  */
    hcd -> ux_hcd_controller_hardware =  (VOID *) hcd_stm32;

    /* Set the generic HCD owner for the STM32 HCD.  */
    hcd_stm32 -> ux_hcd_stm32_hcd_owner =  hcd;

    /* Initialize the function collector for this HCD.  */
    hcd -> ux_hcd_entry_function =  _ux_hcd_stm32_entry;

    /* Set the state of the controller to HALTED first.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_HALTED;

    /* Initialize the number of channels.  */
    hcd_stm32 -> ux_hcd_stm32_nb_channels =  UX_HCD_STM32_MAX_NB_CHANNELS;

    /* Check if the parameter is null.  */
    if (hcd -> ux_hcd_irq == 0)
    {
        _ux_utility_memory_free(hcd_stm32);
        return(UX_ERROR);
    }

    /* Get HCD handle from parameter.  */
    hcd_stm32 -> hcd_handle = (HCD_HandleTypeDef*)hcd -> ux_hcd_irq;
    hcd_stm32 -> hcd_handle -> pData = hcd;

    /* Allocate the list of eds.   */
    hcd_stm32 -> ux_hcd_stm32_ed_list =  _ux_utility_memory_allocate(UX_NO_ALIGN, UX_REGULAR_MEMORY, sizeof(UX_HCD_STM32_ED) *_ux_system_host -> ux_system_host_max_ed);
    if (hcd_stm32 -> ux_hcd_stm32_ed_list == UX_NULL)
    {
        _ux_utility_memory_free(hcd_stm32);
        return(UX_MEMORY_INSUFFICIENT);
    }

    /* Since we know this is a high-speed controller, we can hardwire the version.  */
#if UX_MAX_DEVICES > 1
    hcd -> ux_hcd_version =  0x200;
#endif

    /* The number of ports on the controller is fixed to 1. The number of ports needs to be reflected both
       for the generic HCD container and the local stm32 container.  */
    hcd -> ux_hcd_nb_root_hubs             =  UX_HCD_STM32_NB_ROOT_PORTS;

    /* The root port must now be powered to pick up device insertion.  */
    _ux_hcd_stm32_power_on_port(hcd_stm32, 0);

    /* The asynchronous queues are empty for now.  */
    hcd_stm32 -> ux_hcd_stm32_queue_empty =  UX_TRUE;

    /* The periodic scheduler is not active.  */
    hcd_stm32 -> ux_hcd_stm32_periodic_scheduler_active =  0;

    /* Set the host controller into the operational state.  */
    hcd -> ux_hcd_status =  UX_HCD_STATUS_OPERATIONAL;

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_entry(UX_HCD *hcd, UINT function, VOID *parameter)
{

UINT                status;
UX_HCD_STM32       *hcd_stm32;
UX_INTERRUPT_SAVE_AREA


    /* Check the status of the controller.  */
    if (hcd -> ux_hcd_status == UX_UNUSED)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_CONTROLLER_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_CONTROLLER_UNKNOWN, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_CONTROLLER_UNKNOWN);
    }

    /* Get the pointer to the STM32 HCD.  */
    hcd_stm32 =  (UX_HCD_STM32 *) hcd -> ux_hcd_controller_hardware;

    /* look at the function and route it.  */
    switch(function)
    {

    case UX_HCD_DISABLE_CONTROLLER:

        status =  _ux_hcd_stm32_controller_disable(hcd_stm32);
        break;


    case UX_HCD_GET_PORT_STATUS:

        status =  _ux_hcd_stm32_port_status_get(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_ENABLE_PORT:

        status =  _ux_hcd_stm32_port_enable(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_DISABLE_PORT:

        status =  _ux_hcd_stm32_port_disable(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_POWER_ON_PORT:

        status =  _ux_hcd_stm32_power_on_port(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_POWER_DOWN_PORT:

        status =  _ux_hcd_stm32_power_down_port(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_SUSPEND_PORT:

        status =  _ux_hcd_stm32_port_suspend(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_RESUME_PORT:

        status =  _ux_hcd_stm32_port_resume(hcd_stm32, (UINT) parameter);
        break;


    case UX_HCD_RESET_PORT:

        status =  _ux_hcd_stm32_port_reset(hcd_stm32, (ULONG) parameter);
        break;


    case UX_HCD_GET_FRAME_NUMBER:

        status =  _ux_hcd_stm32_frame_number_get(hcd_stm32, (ULONG *) parameter);
        break;


    case UX_HCD_TRANSFER_REQUEST:

        status =  _ux_hcd_stm32_request_transfer(hcd_stm32, (UX_TRANSFER *) parameter);
        break;


    case UX_HCD_TRANSFER_ABORT:

        status =  _ux_hcd_stm32_transfer_abort(hcd_stm32, (UX_TRANSFER *) parameter);
        break;


    case UX_HCD_CREATE_ENDPOINT:

        status =  _ux_hcd_stm32_endpoint_create(hcd_stm32, (UX_ENDPOINT*) parameter);
        break;

    case UX_HCD_DESTROY_ENDPOINT:

        status =  _ux_hcd_stm32_endpoint_destroy(hcd_stm32, (UX_ENDPOINT*) parameter);
        break;

    case UX_HCD_RESET_ENDPOINT:

        status =  _ux_hcd_stm32_endpoint_reset(hcd_stm32, (UX_ENDPOINT*) parameter);
        break;

    case UX_HCD_PROCESS_DONE_QUEUE:

        /* Process periodic queue.  */
        _ux_hcd_stm32_periodic_schedule(hcd_stm32);

        /* Reset the SOF flag.  */
        UX_DISABLE
        hcd_stm32 -> ux_hcd_stm32_controller_flag &= ~UX_HCD_STM32_CONTROLLER_FLAG_SOF;
        UX_RESTORE

        status =  UX_SUCCESS;
        break;

    case UX_HCD_UNINITIALIZE:

        /* free HCD resources */
        if (hcd_stm32 != UX_NULL)
        {
          _ux_utility_memory_free(hcd_stm32 -> ux_hcd_stm32_ed_list);
          _ux_utility_memory_free(hcd_stm32);
        }

        status =  UX_SUCCESS;
        break;

    default:

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_FUNCTION_NOT_SUPPORTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_FUNCTION_NOT_SUPPORTED, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        /* Unknown request, return an error.  */
        status =  UX_FUNCTION_NOT_SUPPORTED;
        break;

    }

    /* Return completion status.  */
    return(status);
}

UINT  _ux_hcd_stm32_periodic_schedule(UX_HCD_STM32 *hcd_stm32)
{

UX_HCD_STM32_ED     *ed;
UX_TRANSFER         *transfer_request;
ULONG               frame_index;
#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
UX_DEVICE           *parent_device;
UX_ENDPOINT         *endpoint;
UX_ENDPOINT         *parent_endpoint;
ULONG               ep_schedule = 1U;
USHORT              port_status_change_bits;
#endif /* defined (USBH_HAL_HUB_SPLIT_SUPPORTED) */

    /* Get the current frame number.  */
    frame_index = HAL_HCD_GetCurrentFrame(hcd_stm32 -> hcd_handle);

    /* Get the first ED in the periodic list.  */
    ed =  hcd_stm32 -> ux_hcd_stm32_periodic_ed_head;

    /* Search for an entry in the periodic tree.  */
    while (ed != UX_NULL)
    {
#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
      if (hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].do_ssplit == 1U)
      {
        /* Get the transfer request.  */
        transfer_request = ed -> ux_stm32_ed_transfer_request;

        if (transfer_request != NULL)
        {
          if ((frame_index & ed -> ux_stm32_ed_interval_mask) == ed -> ux_stm32_ed_interval_position)
          {
            hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].ep_ss_schedule = 1U;
          }

          /* Schedule Start & Complete split where the entire split transaction is completely bounded by a
          frame FS/LS devices */
          if (((((frame_index & 0x7U) < 0x3U) || ((frame_index & 0x7U) == 0x7U)) &&
               (hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].ep_ss_schedule == 1U)) ||
                ((hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].do_csplit == 1U) &&
                 (frame_index > (ed -> ux_stm32_ed_current_ss_frame + 1U))))
          {
            if (hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].ep_ss_schedule == 1U)
            {
              hcd_stm32 -> hcd_handle -> hc[ed -> ux_stm32_ed_channel].ep_ss_schedule = 0U;
              ed -> ux_stm32_ed_current_ss_frame = frame_index;
            }

            /* Check if there is transfer needs schedule.  */
            if (ed -> ux_stm32_ed_sch_mode)
            {
              /* If it's scheduled each SOF/uSOF, the request should be submitted
              * immediately after packet is done. This is performed in callback.  */
              if (ed -> ux_stm32_ed_interval_mask == 0)
                ed -> ux_stm32_ed_sch_mode = 0;

              /* For ISO OUT, packet size is from request variable,
              * otherwise, use request length.  */
              if ((ed -> ux_stm32_ed_type == EP_TYPE_ISOC) && (ed -> ux_stm32_ed_dir == 0U))
                ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_packet_length;
              else
                ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_requested_length;

              /* Prepare transactions.  */
              _ux_hcd_stm32_request_trans_prepare(hcd_stm32, ed, transfer_request);

              /* Get the pointer to the Endpoint.  */
              endpoint = (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

              /* Check if device connected to hub  */
              if (endpoint->ux_endpoint_device->ux_device_parent != NULL)
              {
                parent_device = endpoint->ux_endpoint_device->ux_device_parent;
                if (parent_device->ux_device_current_configuration->ux_configuration_first_interface->ux_interface_descriptor.bInterfaceClass == 0x9U)
                {
                  parent_endpoint = parent_device->ux_device_current_configuration->ux_configuration_first_interface->ux_interface_first_endpoint;

                  if (parent_endpoint->ux_endpoint_transfer_request.ux_transfer_request_actual_length != 0U)
                  {
                    /* The interrupt pipe buffer contains the status change for each of the ports
                    the length of the buffer can be 1 or 2 depending on the number of ports.
                    Usually, since HUBs can be bus powered the maximum number of ports is 4.
                    We must be taking precautions on how we read the buffer content for
                    big endian machines.  */
                    if (parent_endpoint->ux_endpoint_transfer_request.ux_transfer_request_actual_length == 1)
                      port_status_change_bits = *(USHORT *) parent_endpoint->ux_endpoint_transfer_request.ux_transfer_request_data_pointer;
                    else
                      port_status_change_bits = (USHORT)_ux_utility_short_get(parent_endpoint->ux_endpoint_transfer_request.ux_transfer_request_data_pointer);

                    if ((port_status_change_bits & (0x1U << endpoint->ux_endpoint_device->ux_device_port_location)) != 0U)
                    {
                      ep_schedule = 0U;
                    }
                  }
                }
              }

              if ((endpoint->ux_endpoint_device->ux_device_state == UX_DEVICE_CONFIGURED) && (ep_schedule != 0U))
              {
                /* Call HAL driver to submit the transfer request.  */
                HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                         ed -> ux_stm32_ed_dir,
                                         ed -> ux_stm32_ed_type, USBH_PID_DATA,
                                         ed -> ux_stm32_ed_data + transfer_request -> ux_transfer_request_actual_length,
                                         ed -> ux_stm32_ed_packet_length, 0);
              }
            }
          }
        }
      }
      else
#endif /* defined (USBH_HAL_HUB_SPLIT_SUPPORTED) */
      {
        /* Check if the periodic transfer should be scheduled in this frame.  */
        /* Interval Mask is 0:     it's scheduled every SOF/uSOF.  */
        /* Interval Mask is not 0: check position to see if it's scheduled.  */
        if ((frame_index & ed -> ux_stm32_ed_interval_mask) == ed -> ux_stm32_ed_interval_position)
        {

          /* Get the transfer request.  */
          transfer_request = ed -> ux_stm32_ed_transfer_request;

          /* Check if there is transfer needs schedule.  */
          if (transfer_request && ed -> ux_stm32_ed_sch_mode)
          {

            /* If it's scheduled each SOF/uSOF, the request should be submitted
            * immediately after packet is done. This is performed in callback.  */
            if (ed -> ux_stm32_ed_interval_mask == 0)
              ed -> ux_stm32_ed_sch_mode = 0;

            /* For ISO OUT, packet size is from request variable,
            * otherwise, use request length.  */
            if ((ed -> ux_stm32_ed_type == EP_TYPE_ISOC) && (ed -> ux_stm32_ed_dir == 0U))
              ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_packet_length;
            else
              ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_requested_length;

            /* Prepare transactions.  */
            _ux_hcd_stm32_request_trans_prepare(hcd_stm32, ed, transfer_request);


            /* Call HAL driver to submit the transfer request.  */
            HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                     ed -> ux_stm32_ed_dir,
                                     ed -> ux_stm32_ed_type, USBH_PID_DATA,
                                     transfer_request -> ux_transfer_request_data_pointer +
                                       transfer_request -> ux_transfer_request_actual_length,
                                       ed -> ux_stm32_ed_packet_length, 0);
          }
        }
      }

        /* Point to the next ED in the list.  */
        ed =  ed -> ux_stm32_ed_next_ed;
    }

    /* Return to caller.  */
    return(UX_FALSE);
}

UINT  _ux_hcd_stm32_port_disable(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_port_enable(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Return successful completion.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_port_reset(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Check to see if this port is valid on this controller.  On STM32, there is only one. */
    if (port_index != 0)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_PORT_INDEX_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_PORT_INDEX_UNKNOWN, port_index, 0, 0, UX_TRACE_ERRORS, 0, 0)

#if defined(UX_HOST_STANDALONE)
        return(UX_STATE_ERROR);
#else
        return(UX_PORT_INDEX_UNKNOWN);
#endif /* defined(UX_HOST_STANDALONE) */
    }

    /* Ensure that the downstream port has a device attached. It is unnatural
       to perform a port reset if there is no device.  */
    if ((hcd_stm32 -> ux_hcd_stm32_controller_flag & UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED) == 0)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_NO_DEVICE_CONNECTED);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_NO_DEVICE_CONNECTED, port_index, 0, 0, UX_TRACE_ERRORS, 0, 0)

#if defined(UX_HOST_STANDALONE)
        return(UX_STATE_ERROR);
#else
        return(UX_NO_DEVICE_CONNECTED);
#endif /* defined(UX_HOST_STANDALONE) */
    }

#if defined(UX_HOST_STANDALONE)
    /* There is no way for non-blocking reset in HCD, just do blocking operation here.  */
    HAL_HCD_ResetPort(hcd_stm32 -> hcd_handle);
    return(UX_STATE_NEXT);
#else
    HAL_HCD_ResetPort(hcd_stm32 -> hcd_handle);

    /* This function should never fail.  */
    return(UX_SUCCESS);
#endif /* defined(UX_HOST_STANDALONE) */

}

UINT  _ux_hcd_stm32_port_resume(UX_HCD_STM32 *hcd_stm32, UINT port_index)
{

    /* Return error status.  */
    return(UX_FUNCTION_NOT_SUPPORTED);
}

ULONG  _ux_hcd_stm32_port_status_get(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

ULONG       port_status;


    /* Check to see if this port is valid on this controller.  */
    if (UX_HCD_STM32_NB_ROOT_PORTS < port_index)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_PORT_INDEX_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_PORT_INDEX_UNKNOWN, port_index, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_PORT_INDEX_UNKNOWN);
    }

    /* The port is valid, build the status mask for this port. This function
       returns a controller agnostic bit field.  */
    port_status =  0;

    /* Device Connection Status.  */
    if (hcd_stm32 -> ux_hcd_stm32_controller_flag & UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED)
        port_status |=  UX_PS_CCS;

    switch (HAL_HCD_GetCurrentSpeed(hcd_stm32 -> hcd_handle))
    {
    case 0:
        /* High Speed.  */
        port_status |=  UX_PS_DS_HS;
        break;

    case 1:
        /* Full Speed.  */
        port_status |=  UX_PS_DS_FS;
        break;

    case 2:
        /* Low Speed.  */
        port_status |=  UX_PS_DS_LS;
        break;

    default:
        /* Full Speed.  */
        port_status |=  UX_PS_DS_FS;
        break;
    }

    /* Return port status.  */
    return(port_status);
}

UINT  _ux_hcd_stm32_port_suspend(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Return error status.  */
    return(UX_FUNCTION_NOT_SUPPORTED);
}

UINT  _ux_hcd_stm32_power_down_port(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Return error status.  */
    return(UX_FUNCTION_NOT_SUPPORTED);
}

UINT  _ux_hcd_stm32_power_on_port(UX_HCD_STM32 *hcd_stm32, ULONG port_index)
{

    /* Check to see if this port is valid on this controller.  On STM32, there is only one. */
    if (port_index != 0)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_PORT_INDEX_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_PORT_INDEX_UNKNOWN, port_index, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_PORT_INDEX_UNKNOWN);
    }

    /* This function never fails.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_request_bulk_transfer(UX_HCD_STM32 *hcd_stm32, UX_TRANSFER *transfer_request)
{

#if defined(UX_HOST_STANDALONE)
UX_INTERRUPT_SAVE_AREA
#endif /* defined(UX_HOST_STANDALONE) */
UX_ENDPOINT         *endpoint;
UX_HCD_STM32_ED     *ed;
UINT                direction;
UINT                length;

    /* Get the pointer to the Endpoint.  */
    endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

    /* Now get the physical ED attached to this endpoint.  */
    ed =  endpoint -> ux_endpoint_ed;

#if defined(UX_HOST_STANDALONE)
    UX_DISABLE

    /* Check if transfer is still in progress.  */
    if ((ed -> ux_stm32_ed_status & UX_HCD_STM32_ED_STATUS_PENDING_MASK) >
        UX_HCD_STM32_ED_STATUS_ABORTED)
    {

        /* Check done bit.  */
        if ((ed -> ux_stm32_ed_status & UX_HCD_STM32_ED_STATUS_TRANSFER_DONE) == 0)
        {
            UX_RESTORE
            return(UX_STATE_WAIT);
        }

        /* Check status to see if it's first initialize.  */
        if (transfer_request -> ux_transfer_request_status !=
            UX_TRANSFER_STATUS_NOT_PENDING)
        {

            /* Done, modify status and notify state change.  */
            ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ALLOCATED;
            UX_RESTORE
            return(UX_STATE_NEXT);
        }

        /* Maybe transfer completed but state not reported yet.  */
    }
    transfer_request -> ux_transfer_request_status = UX_TRANSFER_STATUS_PENDING;

    UX_RESTORE
#endif /* defined(UX_HOST_STANDALONE) */

    /* Save the pending transfer in the ED.  */
    ed -> ux_stm32_ed_transfer_request = transfer_request;

    /* Direction, 0 : Output / 1 : Input */
    direction = ed -> ux_stm32_ed_dir;

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
    if (hcd_stm32->hcd_handle->hc[ed -> ux_stm32_ed_channel].do_ssplit == 1U)
    {
      if ((direction == 0) && (transfer_request -> ux_transfer_request_requested_length > endpoint -> ux_endpoint_descriptor.wMaxPacketSize))
      {
        /* Set transfer length to MPS.  */
        length = endpoint -> ux_endpoint_descriptor.wMaxPacketSize;
      }
      else
      {
        /* Keep the original transfer length.  */
        length = transfer_request -> ux_transfer_request_requested_length;
      }
    }
    else
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */
    {

      /* If DMA enabled, use max possible transfer length.  */
      if (hcd_stm32 -> hcd_handle -> Init.dma_enable)
      {
        if (transfer_request -> ux_transfer_request_requested_length > endpoint -> ux_endpoint_transfer_request.ux_transfer_request_maximum_length)
          length = endpoint -> ux_endpoint_transfer_request.ux_transfer_request_maximum_length;
        else
          length = transfer_request -> ux_transfer_request_requested_length;
      }
      else
      {
        /* If the direction is OUT, request size is larger than MPS, and DMA is not used, we need to set transfer length to MPS.  */
        if ((direction == 0) && (transfer_request -> ux_transfer_request_requested_length > endpoint -> ux_endpoint_descriptor.wMaxPacketSize))
        {

          /* Set transfer length to MPS.  */
          length = endpoint -> ux_endpoint_descriptor.wMaxPacketSize;
        }
        else
        {

          /* Keep the original transfer length.  */
          length = transfer_request -> ux_transfer_request_requested_length;
        }
      }
    }

    /* Save the transfer status in the ED.  */
    ed -> ux_stm32_ed_status = direction == 0 ? UX_HCD_STM32_ED_STATUS_BULK_OUT : UX_HCD_STM32_ED_STATUS_BULK_IN;

    /* Save the transfer length.  */
    ed -> ux_stm32_ed_packet_length = length;

    /* Prepare transactions.  */
    _ux_hcd_stm32_request_trans_prepare(hcd_stm32, ed, transfer_request);

    /* Submit the transfer request.  */
    HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                             direction,
                             EP_TYPE_BULK, USBH_PID_DATA,
                             ed -> ux_stm32_ed_data,
                             length, 0);

#if defined(UX_HOST_STANDALONE)

    /* Background transfer started but not done yet.  */
    return(UX_STATE_WAIT);
#else

    /* Return successful completion.  */
    return(UX_SUCCESS);
#endif /* defined(UX_HOST_STANDALONE) */

}

UINT  _ux_hcd_stm32_request_control_transfer(UX_HCD_STM32 *hcd_stm32, UX_TRANSFER *transfer_request)
{

#if defined(UX_HOST_STANDALONE)
UX_INTERRUPT_SAVE_AREA
#else
UINT                    status;
#endif /* defined(UX_HOST_STANDALONE) */
UX_ENDPOINT             *endpoint;
UX_HCD_STM32_ED         *ed;

    /* Get the pointer to the Endpoint.  */
    endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

    /* Now get the physical ED attached to this endpoint.  */
    ed =  endpoint -> ux_endpoint_ed;

#if defined(UX_HOST_STANDALONE)
    UX_DISABLE
    switch(ed -> ux_stm32_ed_status)
    {
    case UX_HCD_STM32_ED_STATUS_ALLOCATED:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_ABORTED:

        /* Setup for SETUP packet.  */
        _ux_hcd_stm32_request_control_setup(hcd_stm32, ed, endpoint, transfer_request);
        UX_RESTORE
        if (ed -> ux_stm32_ed_setup == UX_NULL)
        {
            transfer_request -> ux_transfer_request_completion_code = UX_MEMORY_INSUFFICIENT;
            return(UX_STATE_ERROR);
        }
        return(UX_STATE_WAIT);

    case UX_HCD_STM32_ED_STATUS_CONTROL_SETUP | UX_HCD_STM32_ED_STATUS_TRANSFER_DONE:

        /* Free allocated memory.  */
        _ux_utility_memory_free(ed -> ux_stm32_ed_setup);
        ed -> ux_stm32_ed_setup = UX_NULL;

        /* Restore request information.  */
        transfer_request -> ux_transfer_request_requested_length =
                                                ed -> ux_stm32_ed_saved_length;
        transfer_request -> ux_transfer_request_actual_length = 0;

        /* Check completion code.  */
        if (transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
        {
            ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ALLOCATED;
            UX_RESTORE
            return(UX_STATE_NEXT);
        }

        if (ed -> ux_stm32_ed_saved_length)
        {

            /* To data stage.  */
            _ux_hcd_stm32_request_control_data(hcd_stm32, ed, endpoint, transfer_request);
        }
        else
        {

            /* To status stage.  */
            _ux_hcd_stm32_request_control_status(hcd_stm32, ed, endpoint, transfer_request);
        }
        UX_RESTORE
        return(UX_STATE_WAIT);

    case UX_HCD_STM32_ED_STATUS_CONTROL_DATA_IN | UX_HCD_STM32_ED_STATUS_TRANSFER_DONE:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_DATA_OUT | UX_HCD_STM32_ED_STATUS_TRANSFER_DONE:

        /* Check completion code.  */
        if (transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
        {
            ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ALLOCATED;
            UX_RESTORE
            return(UX_STATE_NEXT);
        }

        /* Get request actual length for IN transfer.  */
        if (ed -> ux_stm32_ed_dir)
        {

            /* Get the actual transfer length.  */
            transfer_request -> ux_transfer_request_actual_length =
                            HAL_HCD_HC_GetXferCount(hcd_stm32 -> hcd_handle,
                                                    ed -> ux_stm32_ed_channel);
        }
        else
        {

            /* For OUT, all data is sent.  */
            transfer_request -> ux_transfer_request_actual_length =
                    transfer_request -> ux_transfer_request_requested_length;
        }

        /* To status stage.  */
        _ux_hcd_stm32_request_control_status(hcd_stm32, ed, endpoint, transfer_request);
        UX_RESTORE
        return(UX_STATE_WAIT);

    case UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_IN | UX_HCD_STM32_ED_STATUS_TRANSFER_DONE:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_OUT | UX_HCD_STM32_ED_STATUS_TRANSFER_DONE:

        /* All done, reset status.  */
        ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ALLOCATED;

        /* Restore saved things.  */
        transfer_request -> ux_transfer_request_requested_length = ed -> ux_stm32_ed_saved_length;
        transfer_request -> ux_transfer_request_actual_length = ed -> ux_stm32_ed_saved_actual_length;

        UX_RESTORE
        return(UX_STATE_NEXT);

    case UX_HCD_STM32_ED_STATUS_CONTROL_SETUP:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_DATA_IN:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_DATA_OUT:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_IN:
    /* Fall through.  */
    case UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_OUT:

        /* Keep waiting.  */
        UX_RESTORE
        return(UX_STATE_WAIT);

    default:
        UX_RESTORE

        /* Error trap.  */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_INVALID_STATE);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_INVALID_STATE, 0, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_STATE_EXIT);
    }
#else

    /* Setup for SETUP packet.  */
    _ux_hcd_stm32_request_control_setup(hcd_stm32, ed, endpoint, transfer_request);
    if (ed -> ux_stm32_ed_setup == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* Wait for the completion of the transfer request.  */
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_CONTROL_TRANSFER_TIMEOUT));

    /* Free the resources.  */
    _ux_utility_memory_free(ed -> ux_stm32_ed_setup);

    /* If the semaphore did not succeed we probably have a time out.  */
    if (status != UX_SUCCESS)
    {

        /* All transfers pending need to abort. There may have been a partial transfer.  */
        _ux_host_stack_transfer_request_abort(transfer_request);

        /* There was an error, return to the caller.  */
        transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_TRANSFER_TIMEOUT, transfer_request, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_TRANSFER_TIMEOUT);
    }

    /* Check the transfer request completion code.  */
    if (transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
    {

        /* Return completion to caller.  */
        return(transfer_request -> ux_transfer_request_completion_code);
    }

    /* Check if there is data phase.  */
    if (ed -> ux_stm32_ed_saved_length)
    {

        /* Prepare data stage.  */
        _ux_hcd_stm32_request_control_data(hcd_stm32, ed, endpoint, transfer_request);

        /* Wait for the completion of the transfer request.  */
        status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, MS_TO_TICK(UX_CONTROL_TRANSFER_TIMEOUT));

        /* If the semaphore did not succeed we probably have a time out.  */
        if (status != UX_SUCCESS)
        {

            /* All transfers pending need to abort. There may have been a partial transfer.  */
            _ux_host_stack_transfer_request_abort(transfer_request);

            /* There was an error, return to the caller.  */
            transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

            /* If trace is enabled, insert this event into the trace buffer.  */
            UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_TRANSFER_TIMEOUT, transfer_request, 0, 0, UX_TRACE_ERRORS, 0, 0)

            return(UX_TRANSFER_TIMEOUT);

        }

        /* Check the transfer request completion code.  */
        if (transfer_request -> ux_transfer_request_completion_code != UX_SUCCESS)
        {

            /* Return completion to caller.  */
            return(transfer_request -> ux_transfer_request_completion_code);
        }
    }

    /* Prepare status stage.  */
    _ux_hcd_stm32_request_control_status(hcd_stm32, ed, endpoint, transfer_request);

    /* Wait for the completion of the transfer request.  */
    status =  _ux_host_semaphore_get(&transfer_request -> ux_transfer_request_semaphore, UX_MS_TO_TICK(UX_CONTROL_TRANSFER_TIMEOUT));

    /* Restore the original transfer parameter.  */
    transfer_request -> ux_transfer_request_requested_length = ed -> ux_stm32_ed_saved_length;
    transfer_request -> ux_transfer_request_actual_length    = ed -> ux_stm32_ed_saved_actual_length;

    /* If the semaphore did not succeed we probably have a time out.  */
    if (status != UX_SUCCESS)
    {

        /* All transfers pending need to abort. There may have been a partial transfer.  */
        _ux_host_stack_transfer_request_abort(transfer_request);

        /* There was an error, return to the caller.  */
        transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_TIMEOUT;

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_TRANSFER_TIMEOUT, transfer_request, 0, 0, UX_TRACE_ERRORS, 0, 0)

    }

    /* Return completion to caller.  */
    return(transfer_request -> ux_transfer_request_completion_code);
#endif /* defined(UX_HOST_STANDALONE) */
}

static inline VOID _ux_hcd_stm32_request_control_setup(UX_HCD_STM32 *hcd_stm32,
    UX_HCD_STM32_ED *ed, UX_ENDPOINT *endpoint, UX_TRANSFER *transfer_request)
{
UCHAR                   *setup_request;

    /* Save the pending transfer in the ED.  */
    ed -> ux_stm32_ed_transfer_request = transfer_request;

    /* Build the SETUP packet (phase 1 of the control transfer).  */
    ed -> ux_stm32_ed_setup = UX_NULL;
    setup_request = _ux_utility_memory_allocate(UX_NO_ALIGN, UX_CACHE_SAFE_MEMORY, UX_SETUP_SIZE);

    if (setup_request == UX_NULL)
        return;

    ed -> ux_stm32_ed_setup = setup_request;

    /* Build the SETUP request.  */
    *(setup_request + UX_SETUP_REQUEST_TYPE) =  transfer_request -> ux_transfer_request_type;
    *(setup_request + UX_SETUP_REQUEST) =       transfer_request -> ux_transfer_request_function;
    _ux_utility_short_put(setup_request + UX_SETUP_VALUE, transfer_request -> ux_transfer_request_value);
    _ux_utility_short_put(setup_request + UX_SETUP_INDEX, transfer_request -> ux_transfer_request_index);
    _ux_utility_short_put(setup_request + UX_SETUP_LENGTH, (USHORT) transfer_request -> ux_transfer_request_requested_length);

    /* Save the original transfer parameter.  */
    ed -> ux_stm32_ed_saved_length = transfer_request -> ux_transfer_request_requested_length;
    ed -> ux_stm32_ed_data = setup_request;

    /* Reset requested length for SETUP packet.  */
    transfer_request -> ux_transfer_request_requested_length = 0;

    /* Set the packet length for SETUP packet.  */
    ed -> ux_stm32_ed_packet_length = 8;

    /* Set the current status.  */
    ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_CONTROL_SETUP;

    /* Set device speed.  */
    switch (endpoint -> ux_endpoint_device -> ux_device_speed)
    {
    case UX_HIGH_SPEED_DEVICE:
        ed -> ux_stm32_ed_speed =  HCD_DEVICE_SPEED_HIGH;
        break;
    case UX_LOW_SPEED_DEVICE:
        ed -> ux_stm32_ed_speed =  HCD_DEVICE_SPEED_LOW;
        break;
    default:
        ed -> ux_stm32_ed_speed =  HCD_DEVICE_SPEED_FULL;
        break;
    }

    /* Initialize the host channel for SETUP phase.  */
    ed -> ux_stm32_ed_dir = 0;
    HAL_HCD_HC_Init(hcd_stm32 -> hcd_handle,
                    ed -> ux_stm32_ed_channel,
                    0,
                    endpoint -> ux_endpoint_device -> ux_device_address,
                    ed -> ux_stm32_ed_speed,
                    EP_TYPE_CTRL,
                    endpoint -> ux_endpoint_descriptor.wMaxPacketSize);

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
    /* Check if device connected to hub  */
    if (endpoint->ux_endpoint_device->ux_device_parent != NULL)
    {
      HAL_HCD_HC_SetHubInfo(hcd_stm32->hcd_handle, ed->ux_stm32_ed_channel,
                            endpoint->ux_endpoint_device->ux_device_parent->ux_device_address,
                            endpoint->ux_endpoint_device->ux_device_port_location);
    }
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */

    /* Send the SETUP packet.  */
    HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                             0, EP_TYPE_CTRL, USBH_PID_SETUP, setup_request, 8, 0);
}

static inline VOID _ux_hcd_stm32_request_control_data(UX_HCD_STM32 *hcd_stm32,
    UX_HCD_STM32_ED *ed, UX_ENDPOINT *endpoint, UX_TRANSFER *transfer_request)
{

    /* Check the direction of the transaction.  */
    if ((transfer_request -> ux_transfer_request_type & UX_REQUEST_DIRECTION) ==
         UX_REQUEST_IN)
    {

        /* Re-initialize the host channel to IN direction.  */
        ed -> ux_stm32_ed_dir = 1;
        HAL_HCD_HC_Init(hcd_stm32 -> hcd_handle,
                        ed -> ux_stm32_ed_channel,
                        0x80,
                        endpoint -> ux_endpoint_device -> ux_device_address,
                        ed -> ux_stm32_ed_speed,
                        EP_TYPE_CTRL,
                        endpoint -> ux_endpoint_descriptor.wMaxPacketSize);

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
        /* Check if device connected to hub  */
        if (endpoint->ux_endpoint_device->ux_device_parent != NULL)
        {
          HAL_HCD_HC_SetHubInfo(hcd_stm32->hcd_handle, ed->ux_stm32_ed_channel,
                                endpoint->ux_endpoint_device->ux_device_parent->ux_device_address,
                                endpoint->ux_endpoint_device->ux_device_port_location);
        }
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */

        /* Set the current status to data IN.  */
        ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_CONTROL_DATA_IN;
    }
    else
    {

        /* Set the current status to data OUT.  */
        ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_CONTROL_DATA_OUT;
    }

    /* Save the pending transfer in the ED.  */
    ed -> ux_stm32_ed_transfer_request = transfer_request;

    /* Set the transfer to pending.  */
    transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_STATUS_PENDING;

    /* Restore requested length.  */
    transfer_request -> ux_transfer_request_requested_length = ed -> ux_stm32_ed_saved_length;

    /* If the direction is OUT, request size is larger than MPS, and DMA is not used, we need to set transfer length to MPS.  */
    if ((ed -> ux_stm32_ed_dir == 0) &&
            (transfer_request -> ux_transfer_request_requested_length > endpoint -> ux_endpoint_descriptor.wMaxPacketSize) &&
            (hcd_stm32 -> hcd_handle -> Init.dma_enable == 0))
    {

        /* Set transfer length to MPS.  */
        ed -> ux_stm32_ed_packet_length = endpoint -> ux_endpoint_descriptor.wMaxPacketSize;
    }
    else
    {

        /* Keep the original transfer length.  */
        ed -> ux_stm32_ed_packet_length = transfer_request -> ux_transfer_request_requested_length;
    }

    /* Reset actual length.  */
    transfer_request -> ux_transfer_request_actual_length = 0;

    /* Prepare transactions.  */
    _ux_hcd_stm32_request_trans_prepare(hcd_stm32, ed, transfer_request);

    /* Submit the transfer request.  */
    HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                                ed -> ux_stm32_ed_dir,
                                EP_TYPE_CTRL, USBH_PID_DATA,
                                ed -> ux_stm32_ed_data,
                                ed -> ux_stm32_ed_packet_length, 0);
}

static inline VOID _ux_hcd_stm32_request_control_status(UX_HCD_STM32 *hcd_stm32,
    UX_HCD_STM32_ED *ed, UX_ENDPOINT *endpoint, UX_TRANSFER *transfer_request)
{

    /* Setup status phase direction.  */
    ed -> ux_stm32_ed_dir = !ed -> ux_stm32_ed_dir;
    HAL_HCD_HC_Init(hcd_stm32 -> hcd_handle,
                ed -> ux_stm32_ed_channel,
                ed -> ux_stm32_ed_dir ? 0x80 : 0,
                endpoint -> ux_endpoint_device -> ux_device_address,
                ed -> ux_stm32_ed_speed,
                EP_TYPE_CTRL,
                endpoint -> ux_endpoint_descriptor.wMaxPacketSize);

#if defined (USBH_HAL_HUB_SPLIT_SUPPORTED)
    /* Check if device connected to hub  */
    if (endpoint->ux_endpoint_device->ux_device_parent != NULL)
    {
      HAL_HCD_HC_SetHubInfo(hcd_stm32->hcd_handle, ed->ux_stm32_ed_channel,
                            endpoint->ux_endpoint_device->ux_device_parent->ux_device_address,
                            endpoint->ux_endpoint_device->ux_device_port_location);
    }
#endif /* USBH_HAL_HUB_SPLIT_SUPPORTED */

    /* Save the pending transfer in the ED.  */
    ed -> ux_stm32_ed_transfer_request = transfer_request;

    /* Set the transfer to pending.  */
    transfer_request -> ux_transfer_request_completion_code =  UX_TRANSFER_STATUS_PENDING;

    /* Save the original transfer parameter.  */
    ed -> ux_stm32_ed_saved_length = (USHORT)
                    transfer_request -> ux_transfer_request_requested_length;
    transfer_request -> ux_transfer_request_requested_length = 0;

    ed -> ux_stm32_ed_saved_actual_length = (USHORT)
                    transfer_request -> ux_transfer_request_actual_length;
    transfer_request -> ux_transfer_request_actual_length = 0;

    /* Reset the packet length.  */
    ed -> ux_stm32_ed_packet_length = 0;

    /* Set the current status to data OUT.  */
    ed -> ux_stm32_ed_status = ed -> ux_stm32_ed_dir ?
                                    UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_IN :
                                    UX_HCD_STM32_ED_STATUS_CONTROL_STATUS_OUT;

    /* Submit the request for status phase.  */
    HAL_HCD_HC_SubmitRequest(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel,
                             ed -> ux_stm32_ed_dir,
                             EP_TYPE_CTRL, USBH_PID_DATA, 0, 0, 0);
}

UINT  _ux_hcd_stm32_request_periodic_transfer(UX_HCD_STM32 *hcd_stm32, UX_TRANSFER *transfer_request)
{

UX_ENDPOINT             *endpoint;
UX_HCD_STM32_ED         *ed;
UX_TRANSFER             *transfer;
UX_INTERRUPT_SAVE_AREA


    /* Get the pointer to the Endpoint.  */
    endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

    /* Now get the physical ED attached to this endpoint.  */
    ed =  endpoint -> ux_endpoint_ed;

    /* Disable interrupt.  */
    UX_DISABLE

#if defined(UX_HOST_STANDALONE)

    /* Check if transfer is still in progress.  */
    if ((ed -> ux_stm32_ed_status & UX_HCD_STM32_ED_STATUS_PENDING_MASK) >
        UX_HCD_STM32_ED_STATUS_ABORTED)
    {

        /* Check done bit.  */
        if ((ed -> ux_stm32_ed_status & UX_HCD_STM32_ED_STATUS_TRANSFER_DONE) == 0)
        {
            UX_RESTORE
            return(UX_STATE_WAIT);
        }

        /* Check status to see if it's first initialize.  */
        if (transfer_request -> ux_transfer_request_status !=
            UX_TRANSFER_STATUS_NOT_PENDING)
        {

            /* Done, modify status and notify state change.  */
            ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ALLOCATED;
            UX_RESTORE
            return(UX_STATE_NEXT);
        }

        /* Maybe transfer completed but state not reported yet.  */
    }
    transfer_request -> ux_transfer_request_status = UX_TRANSFER_STATUS_PENDING;

#endif /* defined(UX_HOST_STANDALONE) */

    /* Save the transfer status in the ED.  */
    ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_PERIODIC_TRANSFER;

    /* Isochronous transfer supports transfer list.  */
    if (ed -> ux_stm32_ed_transfer_request == UX_NULL)
    {

        /* Scheduler is needed to start, and kept if interval is more than 1 SOF/uSOF.  */
        ed -> ux_stm32_ed_sch_mode = 1;

    /* Save the pending transfer in the ED.  */
    ed -> ux_stm32_ed_transfer_request = transfer_request;
    }
    else
    {

        /* Link the pending transfer to list tail.  */
        transfer = ed -> ux_stm32_ed_transfer_request;
        while(transfer -> ux_transfer_request_next_transfer_request != UX_NULL)
            transfer = transfer -> ux_transfer_request_next_transfer_request;
        transfer -> ux_transfer_request_next_transfer_request = transfer_request;
    }

    /* Restore interrupt.  */
    UX_RESTORE

#if defined(UX_HOST_STANDALONE)

    /* Background transfer started but not done yet.  */
    return(UX_STATE_WAIT);
#else

    /* There is no need to wake up the stm32 controller on this transfer
       since periodic transactions will be picked up when the interrupt
       tree is scanned.  */
    return(UX_SUCCESS);
#endif /* defined(UX_HOST_STANDALONE) */

}

VOID  _ux_hcd_stm32_request_trans_finish(UX_HCD_STM32 *hcd_stm32, UX_HCD_STM32_ED *ed)
{
UX_TRANSFER *transfer = ed -> ux_stm32_ed_transfer_request;

    /* If there is no transfer, it's OK.  */
    if (transfer == UX_NULL)
        return;

    /* If there is no data, it's OK.  */
    if (ed -> ux_stm32_ed_data == UX_NULL)
        return;

    /* If the data is aligned, it's OK.  */
    if (ed -> ux_stm32_ed_data == transfer -> ux_transfer_request_data_pointer)
        return;

    /* If the data is IN, copy it.  */
    if (ed -> ux_stm32_ed_dir)
    {
        _ux_utility_memory_copy(transfer -> ux_transfer_request_data_pointer,
                                ed -> ux_stm32_ed_data,
                                transfer -> ux_transfer_request_actual_length);
    }

    /* Free the aligned memory.  */
    _ux_utility_memory_free(ed -> ux_stm32_ed_data);
    ed -> ux_stm32_ed_data = UX_NULL;
}

UINT  _ux_hcd_stm32_request_trans_prepare(UX_HCD_STM32 *hcd_stm32, UX_HCD_STM32_ED *ed, UX_TRANSFER *transfer)
{

    /* Save transfer data pointer.  */
    ed -> ux_stm32_ed_data = transfer -> ux_transfer_request_data_pointer;

    /* If DMA not enabled, nothing to do.  */
    if (!hcd_stm32 -> hcd_handle -> Init.dma_enable)
        return(UX_SUCCESS);

    /* If there is no data, nothing to do.  */
    if (transfer -> ux_transfer_request_requested_length == 0)
        return(UX_SUCCESS);

    /* If transfer buffer aligned, nothing to do.  */
    if (((ALIGN_TYPE)ed -> ux_stm32_ed_data & 0x3UL) == 0)
        return(UX_SUCCESS);

    /* Allocate aligned data buffer for transfer.  */
    ed -> ux_stm32_ed_data = _ux_utility_memory_allocate(UX_NO_ALIGN,
                            UX_CACHE_SAFE_MEMORY,
                            transfer -> ux_transfer_request_requested_length);
    if (ed -> ux_stm32_ed_data == UX_NULL)
        return(UX_MEMORY_INSUFFICIENT);

    /* For data IN it's done.  */
    if (ed -> ux_stm32_ed_dir)
        return(UX_SUCCESS);

    /* For data OUT, copy buffer.  */
    _ux_utility_memory_copy(ed -> ux_stm32_ed_data,
                            transfer -> ux_transfer_request_data_pointer,
                            transfer -> ux_transfer_request_requested_length); /* Use case of memcpy is verified.  */

    /* Done.  */
    return(UX_SUCCESS);
}

UINT  _ux_hcd_stm32_request_transfer(UX_HCD_STM32 *hcd_stm32, UX_TRANSFER *transfer_request)
{

UX_ENDPOINT     *endpoint;
UINT            status;

    /* Device Connection Status.  */
    if (hcd_stm32 -> ux_hcd_stm32_controller_flag & UX_HCD_STM32_CONTROLLER_FLAG_DEVICE_ATTACHED)
    {

        /* Get the pointer to the Endpoint.  */
        endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

#if !defined(UX_HOST_STANDALONE)

        /* We reset the actual length field of the transfer request as a safety measure.  */
        transfer_request -> ux_transfer_request_actual_length =  0;
#endif /* !defined(UX_HOST_STANDALONE) */

        /* Isolate the endpoint type and route the transfer request.  */
        switch ((endpoint -> ux_endpoint_descriptor.bmAttributes) & UX_MASK_ENDPOINT_TYPE)
        {

        case UX_CONTROL_ENDPOINT:

            status = _ux_hcd_stm32_request_control_transfer(hcd_stm32, transfer_request);
            break;

        case UX_BULK_ENDPOINT:

            status = _ux_hcd_stm32_request_bulk_transfer(hcd_stm32, transfer_request);
            break;

        case UX_INTERRUPT_ENDPOINT:
        case UX_ISOCHRONOUS_ENDPOINT:

            status = _ux_hcd_stm32_request_periodic_transfer(hcd_stm32, transfer_request);
            break;

        default:

#if defined(UX_HOST_STANDALONE)
            status =  UX_ERROR;
#else
            transfer_request -> ux_transfer_request_completion_code = UX_ERROR;
            return(UX_STATE_EXIT);
#endif /* defined(UX_HOST_STANDALONE) */
        }
    }
    else
    {

        /* Error, no device attached.  */
#if defined(UX_HOST_STANDALONE)
        status = UX_NO_DEVICE_CONNECTED;
#else
        transfer_request -> ux_transfer_request_completion_code = UX_NO_DEVICE_CONNECTED;
        status = UX_STATE_EXIT;
#endif /* defined(UX_HOST_STANDALONE) */

    }

    return(status);
}

UINT  _ux_hcd_stm32_transfer_abort(UX_HCD_STM32 *hcd_stm32, UX_TRANSFER *transfer_request)
{

UX_ENDPOINT         *endpoint;
UX_HCD_STM32_ED     *ed;
UX_TRANSFER         *transfer;
UX_INTERRUPT_SAVE_AREA

    /* Get the pointer to the endpoint associated with the transfer request.  */
    endpoint =  (UX_ENDPOINT *) transfer_request -> ux_transfer_request_endpoint;

    /* From the endpoint container, get the address of the physical endpoint.  */
    ed =  (UX_HCD_STM32_ED *) endpoint -> ux_endpoint_ed;

    /* Check if this physical endpoint has been initialized properly!  */
    if (ed == UX_NULL)
    {

        /* Error trap. */
        _ux_system_error_handler(UX_SYSTEM_LEVEL_THREAD, UX_SYSTEM_CONTEXT_HCD, UX_ENDPOINT_HANDLE_UNKNOWN);

        /* If trace is enabled, insert this event into the trace buffer.  */
        UX_TRACE_IN_LINE_INSERT(UX_TRACE_ERROR, UX_ENDPOINT_HANDLE_UNKNOWN, endpoint, 0, 0, UX_TRACE_ERRORS, 0, 0)

        return(UX_ENDPOINT_HANDLE_UNKNOWN);
    }

    UX_DISABLE

    /* Halt the host channel.  */
    HAL_HCD_HC_Halt(hcd_stm32 -> hcd_handle, ed -> ux_stm32_ed_channel);

    /* Save the transfer status in the ED.  */
    ed -> ux_stm32_ed_status = UX_HCD_STM32_ED_STATUS_ABORTED;

    /* Finish current transfer.  */
    _ux_hcd_stm32_request_trans_finish(hcd_stm32, ed);

    /* Update the transfer status in linked transfer requests.  */
    transfer = ed -> ux_stm32_ed_transfer_request;
    while(transfer)
    {

        /* Set transfer status to aborted.  */
        transfer -> ux_transfer_request_status = UX_TRANSFER_STATUS_ABORT;

        /* Get next transfer linked.  */
        transfer = transfer -> ux_transfer_request_next_transfer_request;
    }

    /* No transfer on going.  */
    ed -> ux_stm32_ed_transfer_request = UX_NULL;

    UX_RESTORE

#if !defined(UX_HOST_STANDALONE)

    /* Wait for the controller to finish the current frame processing.  */
    _ux_utility_delay_ms(1);
#else

    /* If setup memory is not freed correct, free it.  */
    if (ed -> ux_stm32_ed_setup)
        _ux_utility_memory_free(ed -> ux_stm32_ed_setup);
#endif /* !defined(UX_HOST_STANDALONE) */

    /* Return successful completion.  */
    return(UX_SUCCESS);
}
