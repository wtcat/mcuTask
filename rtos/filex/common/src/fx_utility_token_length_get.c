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
/**   Utility                                                             */
/**                                                                       */
/**************************************************************************/
/**************************************************************************/

#define FX_SOURCE_CODE


/* Include necessary system files.  */

#include "fx_api.h"
#ifdef FX_ENABLE_EXFAT
#include "fx_utility.h"


/**************************************************************************/
/*                                                                        */
/*  FUNCTION                                               RELEASE        */
/*                                                                        */
/*    _fx_utility_token_length_get                        PORTABLE C      */
/*                                                           6.1          */
/*  AUTHOR                                                                */
/*                                                                        */
/*    William E. Lamie, Microsoft Corporation                             */
/*                                                                        */
/*  DESCRIPTION                                                           */
/*                                                                        */
/*    This function returns the length of the next part of the file       */
/*    name.                                                               */
/*                                                                        */
/*  INPUT                                                                 */
/*                                                                        */
/*    path                                  Base (current) path           */
/*                                                                        */
/*  OUTPUT                                                                */
/*                                                                        */
/*    length                                Length of the string          */
/*                                                                        */
/*  CALLS                                                                 */
/*                                                                        */
/*    None                                                                */
/*                                                                        */
/*  CALLED BY                                                             */
/*                                                                        */
/*    _fx_utility_absolute_path_get         Get absolute path             */
/*                                                                        */
/*  RELEASE HISTORY                                                       */
/*                                                                        */
/*    DATE              NAME                      DESCRIPTION             */
/*                                                                        */
/*  05-19-2020     William E. Lamie         Initial Version 6.0           */
/*  09-30-2020     William E. Lamie         Modified comment(s),          */
/*                                            resulting in version 6.1    */
/*                                                                        */
/**************************************************************************/
UINT  _fx_utility_token_length_get(CHAR *path)
{

UINT length;


    /* Initialize the length to 0.  */
    length =  0;

    /* Walk to the next sub-directory break.  */
    while (path[length] && (path[length] != '\\') && (path[length] != '/') && (length < FX_MAXIMUM_PATH))
    {

        /* Increment length (index).  */
        length++;
    }

    /* Return length.  */
    return(length);
}

#endif

