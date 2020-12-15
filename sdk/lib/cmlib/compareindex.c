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

LONG
NTAPI
CmpCompareInIndex(IN PHHIVE Hive,
                  IN PCUNICODE_STRING SearchName,
                  IN ULONG Count,
                  IN PCM_KEY_INDEX Index,
                  IN PHCELL_INDEX SubKey)
{
    PCM_KEY_FAST_INDEX FastIndex;
    PCM_INDEX FastEntry;
    LONG Result;
    ULONG i;
    ULONG ActualNameLength = 4, CompareLength, NameLength;
    WCHAR p, pp;

    /* Assume failure */
    *SubKey = HCELL_NIL;

    /* Check if we are a fast or hashed leaf */
    if ((Index->Signature == CM_KEY_FAST_LEAF) ||
        (Index->Signature == CM_KEY_HASH_LEAF))
    {
        /* Get the Fast/Hash Index */
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        FastEntry = &FastIndex->List[Count];

        /* Check if we are a hash leaf, in which case we skip all this */
        if (Index->Signature == CM_KEY_FAST_LEAF)
        {
            /* Find out just how much of the name is there */
            for (i = 0; i < 4; i++)
            {
                /* Check if this entry is empty */
                if (!FastEntry->NameHint[i])
                {
                    /* Only this much! */
                    ActualNameLength = i;
                    break;
                }
            }

            /* How large is the name and how many characters to compare */
            NameLength = SearchName->Length / sizeof(WCHAR);
            CompareLength = min(NameLength, ActualNameLength);

            /* Loop all the chars we'll test */
            for (i = 0; i < CompareLength; i++)
            {
                /* Get one char from each buffer */
                p = SearchName->Buffer[i];
                pp = FastEntry->NameHint[i];

                /* See if they match and return result if they don't */
                Result = (LONG)RtlUpcaseUnicodeChar(p) -
                         (LONG)RtlUpcaseUnicodeChar(pp);
                if (Result) return (Result > 0) ? 1 : -1;
            }
        }

        /* If we got here then we have to do a full compare */
        Result = CmpDoCompareKeyName(Hive, SearchName, FastEntry->Cell);
        if (Result == 2) return Result;
        if (!Result) *SubKey = FastEntry->Cell;
    }
    else
    {
        /* We aren't, so do a name compare and return the subkey found */
        Result = CmpDoCompareKeyName(Hive, SearchName, Index->List[Count]);
        if (Result == 2) return Result;
        if (!Result) *SubKey = Index->List[Count];
    }

    /* Return the comparison result */
    return Result;
}
