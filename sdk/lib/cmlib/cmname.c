/*
 * PROJECT:         ReactOS Registry Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmname.c
 * PURPOSE:         Configuration Manager - Name Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib_private.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

USHORT
NTAPI
CmpCopyName(IN PHHIVE Hive,
            OUT PWCHAR Destination,
            IN PUNICODE_STRING Source)
{
    ULONG i;

    /* Check for old hives */
    if (Hive->Version == 1)
    {
        /* Just copy the source directly */
        RtlCopyMemory(Destination, Source->Buffer, Source->Length);
        return Source->Length;
    }

    /* For new versions, check for compressed name */
    for (i = 0; i < (Source->Length / sizeof(WCHAR)); i++)
    {
        /* Check if the name is non compressed */
        if (Source->Buffer[i] > (UCHAR)-1)
        {
            /* Do the copy */
            RtlCopyMemory(Destination, Source->Buffer, Source->Length);
            return Source->Length;
        }

        /* Copy this character */
        ((PCHAR)Destination)[i] = (CHAR)(Source->Buffer[i]);
    }

    /* Compressed name, return length */
    return Source->Length / sizeof(WCHAR);
}

USHORT
NTAPI
CmpNameSize(IN PHHIVE Hive,
            IN PUNICODE_STRING Name)
{
    ULONG i;

    /* For old hives, just return the length */
    if (Hive->Version == 1) return Name->Length;

    /* For new versions, check for compressed name */
    for (i = 0; i < (Name->Length / sizeof(WCHAR)); i++)
    {
        /* Check if the name is non compressed */
        if (Name->Buffer[i] > (UCHAR)-1) return Name->Length;
    }

    /* Compressed name, return length */
    return Name->Length / sizeof(WCHAR);
}
