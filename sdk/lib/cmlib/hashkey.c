/*
 * PROJECT:         ReactOS Registry Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmindex.c
 * PURPOSE:         Configuration Manager - Cell Indexes
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib_private.h"
#define NDEBUG
#include <debug.h>

ULONG
NTAPI
CmpComputeHashKey(IN ULONG Hash,
                  IN PCUNICODE_STRING Name,
                  IN BOOLEAN AllowSeparators)
{
    LPWSTR Cp;
    ULONG Value, i;

    /* Make some sanity checks on our parameters */
    ASSERT((Name->Length == 0) ||
           (AllowSeparators) ||
           (Name->Buffer[0] != OBJ_NAME_PATH_SEPARATOR));

    /* If the name is empty, there is nothing to hash! */
    if (!Name->Length) return Hash;

    /* Set the buffer and loop every character */
    Cp = Name->Buffer;
    for (i = 0; i < Name->Length; i += sizeof(WCHAR), Cp++)
    {
        /* Make sure we don't have a separator when we shouldn't */
        ASSERT(AllowSeparators || (*Cp != OBJ_NAME_PATH_SEPARATOR));

        /* Check what kind of char we have */
        if (*Cp >= L'a')
        {
            /* In the lower case region... is it truly lower case? */
            if (*Cp < L'z')
            {
                /* Yes! Calculate it ourselves! */
                Value = *Cp - L'a' + L'A';
            }
            else
            {
                /* No, use the API */
                Value = RtlUpcaseUnicodeChar(*Cp);
            }
        }
        else
        {
            /* Reuse the char, it's already upcased */
            Value = *Cp;
        }

        /* Multiply by a prime and add our value */
        Hash *= 37;
        Hash += Value;
    }

    /* Return the hash */
    return Hash;
}
