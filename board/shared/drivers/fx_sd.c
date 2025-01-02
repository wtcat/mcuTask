/*
 * Copyright 2024 wtcat
 */

#include "fx_api.h"

#include "subsys/sd/private/sd_ops.h"
#include "subsys/sd/sd.h"


VOID _fx_sd_driver(FX_MEDIA *media_ptr) {
	UCHAR *buffer;
	UCHAR *destination_buffer;
	UINT bytes_per_sector;

	/* Process the driver request specified in the media control block.  */
	switch (media_ptr->fx_media_driver_request) {
	case FX_DRIVER_READ:
		media_ptr->fx_media_driver_status = card_read_blocks(
			(struct sd_card *)media_ptr->fx_media_driver_info,
			media_ptr->fx_media_driver_buffer,
			media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors,
			media_ptr->fx_media_driver_sectors
		);
		break;

	case FX_DRIVER_WRITE:
		media_ptr->fx_media_driver_status = card_write_blocks(
			(struct sd_card *)media_ptr->fx_media_driver_info,
			media_ptr->fx_media_driver_buffer,
			media_ptr->fx_media_driver_logical_sector + media_ptr->fx_media_hidden_sectors,
			media_ptr->fx_media_driver_sectors
		);
		break;

	case FX_DRIVER_FLUSH:
		/* Return driver success.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_ABORT:
		/* Return driver success.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_INIT:
		/* FLASH drivers are responsible for setting several fields in the
		   media structure, as follows:

				media_ptr -> fx_media_driver_free_sector_update
				media_ptr -> fx_media_driver_write_protect

		   The fx_media_driver_free_sector_update flag is used to instruct
		   FileX to inform the driver whenever sectors are not being used.
		   This is especially useful for FLASH managers so they don't have
		   maintain mapping for sectors no longer in use.

		   The fx_media_driver_write_protect flag can be set anytime by the
		   driver to indicate the media is not writable.  Write attempts made
		   when this flag is set are returned as errors.  */

		/* Perform basic initialization here... since the boot record is going
		   to be read subsequently and again for volume name requests.  */

		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_UNINIT:
		/* Successful driver request.  */
		media_ptr->fx_media_driver_status = FX_SUCCESS;
		break;

	case FX_DRIVER_BOOT_READ:
		/* Read the boot record and return to the caller.  */
		media_ptr->fx_media_driver_status = card_read_blocks(
			(struct sd_card *)media_ptr->fx_media_driver_info,
			media_ptr->fx_media_driver_buffer,
			0,
			1
		);

		if (media_ptr->fx_media_driver_status == FX_SUCCESS) {
			/* Calculate the RAM disk boot sector offset, which is at the very beginning of
			the RAM disk. Note the RAM disk memory is pointed to by the
			fx_media_driver_info pointer, which is supplied by the application in the
			call to fx_media_open.  */
			buffer = (UCHAR *)media_ptr->fx_media_driver_buffer;

			/* For RAM driver, determine if the boot record is valid.  */
			if ((buffer[0] != (UCHAR)0xEB) ||
				((buffer[1] != (UCHAR)0x34) && (buffer[1] != (UCHAR)0x76)) ||
				(buffer[2] != (UCHAR)0x90)) {
				/* Invalid boot record, return an error!  */
				media_ptr->fx_media_driver_status = FX_MEDIA_INVALID;
				return;
			}

			/* For RAM disk only, pickup the bytes per sector.  */
			bytes_per_sector = _fx_utility_16_unsigned_read(&buffer[FX_BYTES_SECTOR]);

			/* Ensure this is less than the media memory size.  */
			if (bytes_per_sector > media_ptr->fx_media_memory_size) {
				media_ptr->fx_media_driver_status = FX_BUFFER_ERROR;
				break;
			}
		}
		break;

	case FX_DRIVER_BOOT_WRITE:
		/* Write the boot record and return to the caller.  */
		media_ptr->fx_media_driver_status = card_write_blocks(
			(struct sd_card *)media_ptr->fx_media_driver_info,
			media_ptr->fx_media_driver_buffer,
			0,
			1
		);
		break;

	default: 
		/* Invalid driver request.  */
		media_ptr->fx_media_driver_status = FX_IO_ERROR;
		break;
	}
}
