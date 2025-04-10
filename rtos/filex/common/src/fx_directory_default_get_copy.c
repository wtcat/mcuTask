/***************************************************************************
 * Copyright (c) 2024 Microsoft Corporation 
 * 
 * This program and the accompanying materials are made available under the
 * terms of the MIT License which is available at
 * https://opensource.org/licenses/MIT.
 * 
 * SPDX-License-Identifier: MIT
 **************************************************************************/


/**************************************************************************/
/**************************************************************************/
/**                                                                       */
/** FileX Component                                                       */
/**                                                                       */
/**   Directory                                                           */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define FX_SOURCE_CODE


/* Include necessary system files.  */

#include "fx_api.h"
#include "fx_directory.h"
#include "fx_utility.h"

/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _fx_directory_default_get_copy                      PORTABLE C      */
/*                                                           6.1.6        */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function copies the last path provided to the                  */
/*    fx_directory_default_set function into the specified buffer.        */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    media_ptr                             Media control block pointer   */
/*    return_path_name_buffer               Destination buffer for name   */
/*    return_path_name_buffer_size          Size of buffer pointed to by  */
/*                                            return_path_name_buffer     */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    return status                                                       */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    Application Code                                                    */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     William E. Lamie         Initial Version 6.0           */
/*  09-30-2020     William E. Lamie         Modified comment(s), verified */
/*                                            memcpy usage,               */
/*                                            resulting in version 6.1    */
/*  04-02-2021     William E. Lamie         Modified comment(s), verified */
/*                                            memcpy usage,               */
/*                                            resulting in version 6.1.6  */
/*                                                                        */
/**************************************************************************/
UINT  _fx_directory_default_get_copy(FX_MEDIA *media_ptr, CHAR *return_path_name_buffer, UINT return_path_name_buffer_size)
{

UINT    status;
CHAR    *return_path_name;
UINT    path_name_length_with_null_terminator;


    /* Get the pointer to the path.  */
    status =  _fx_directory_default_get(media_ptr, &return_path_name);
    if (status == FX_SUCCESS)
    {

        /* Get the length of the path.  */
        path_name_length_with_null_terminator =  _fx_utility_string_length_get(return_path_name, FX_MAXIMUM_PATH) + 1;

        /* Can it fit in the user's buffer? */
        if (path_name_length_with_null_terminator <= return_path_name_buffer_size)
        {

            /* Copy the path name into the user's buffer.  */
            _fx_utility_memory_copy((UCHAR *) return_path_name, (UCHAR *) return_path_name_buffer, path_name_length_with_null_terminator); /* Use case of memcpy is verified. */
        }
        else
        {

            /* Buffer is too small. Return error.  */
            return(FX_BUFFER_ERROR);
        }

    }

    /* Return successful status.  */
    return(status);
}

