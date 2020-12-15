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

VOID
NTAPI
CmpCopyCompressedName(OUT PWCHAR Destination,
                      IN ULONG DestinationLength,
                      IN PWCHAR Source,
                      IN ULONG SourceLength)
{
    ULONG i, Length;

    /* Get the actual length to copy */
    Length = min(DestinationLength / sizeof(WCHAR), SourceLength);
    for (i = 0; i < Length; i++)
    {
        /* Copy each character */
        Destination[i] = (WCHAR)((PUCHAR)Source)[i];
    }
}

USHORT
NTAPI
CmpCompressedNameSize(IN PWCHAR Name,
                      IN ULONG Length)
{
    /*
     * Don't remove this: compressed names are "opaque" and just because
     * the current implementation turns them into ansi-names doesn't mean
     * that it will remain that way forever, so -never- assume this code
     * below internally!
     */
    return (USHORT)Length * sizeof(WCHAR);
}
