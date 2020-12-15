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

LONG
NTAPI
CmpCompareCompressedName(IN PCUNICODE_STRING SearchName,
                         IN PWCHAR CompressedName,
                         IN ULONG NameLength)
{
    WCHAR* p;
    UCHAR* pp;
    WCHAR chr1, chr2;
    USHORT SearchLength;
    LONG Result;

    /* Set the pointers and length and then loop */
    p = SearchName->Buffer;
    pp = (PUCHAR)CompressedName;
    SearchLength = (SearchName->Length / sizeof(WCHAR));
    while (SearchLength > 0 && NameLength > 0)
    {
        /* Get the characters */
        chr1 = *p++;
        chr2 = (WCHAR)(*pp++);

        /* Check if we have a direct match */
        if (chr1 != chr2)
        {
            /* See if they match and return result if they don't */
            Result = (LONG)RtlUpcaseUnicodeChar(chr1) -
                     (LONG)RtlUpcaseUnicodeChar(chr2);
            if (Result) return Result;
        }

        /* Next chars */
        SearchLength--;
        NameLength--;
    }

    /* Return the difference directly */
    return SearchLength - NameLength;
}

LONG
CMAPI
CmpDoCompareKeyName(IN PHHIVE Hive,
                    IN PCUNICODE_STRING SearchName,
                    IN HCELL_INDEX Cell)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING KeyName;
    LONG Result;

    /* Get the node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, Cell);
    if (!Node) return 2;

    /* Check if it's compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Compare compressed names */
        Result = CmpCompareCompressedName(SearchName,
                                          Node->Name,
                                          Node->NameLength);
    }
    else
    {
        /* Compare the Unicode name directly */
        KeyName.Buffer = Node->Name;
        KeyName.Length = Node->NameLength;
        KeyName.MaximumLength = KeyName.Length;
        Result = RtlCompareUnicodeString(SearchName, &KeyName, TRUE);
    }

    /* Release the cell and return the normalized result */
    HvReleaseCell(Hive, Cell);
    return (Result == 0) ? Result : ((Result > 0) ? 1 : -1);
}
