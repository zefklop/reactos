/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmvalue.c
 * PURPOSE:         Configuration Manager Library - Cell Values
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib_private.h"
#define NDEBUG
#include <debug.h>

BOOLEAN
NTAPI
CmpFindNameInList(IN PHHIVE Hive,
                  IN PCHILD_LIST ChildList,
                  IN PUNICODE_STRING Name,
                  OUT PULONG ChildIndex OPTIONAL,
                  OUT PHCELL_INDEX CellIndex)
{
    PCELL_DATA CellData;
    HCELL_INDEX CellToRelease = HCELL_NIL;
    ULONG i;
    PCM_KEY_VALUE KeyValue;
    LONG Result;
    UNICODE_STRING SearchName;
    BOOLEAN Success;

    /* Make sure there's actually something on the list */
    if (ChildList->Count != 0)
    {
        /* Get the cell data */
        CellData = (PCELL_DATA)HvGetCell(Hive, ChildList->List);
        if (!CellData)
        {
            /* Couldn't get the cell... tell the caller */
            *CellIndex = HCELL_NIL;
            return FALSE;
        }

        /* Now loop every entry */
        for (i = 0; i < ChildList->Count; i++)
        {
            /* Check if we have a cell to release */
            if (CellToRelease != HCELL_NIL)
            {
                /* Release it */
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }

            /* Get this value */
            KeyValue = (PCM_KEY_VALUE)HvGetCell(Hive, CellData->u.KeyList[i]);
            if (!KeyValue)
            {
                /* Return with no data found */
                *CellIndex = HCELL_NIL;
                Success = FALSE;
                goto Return;
            }

            /* Save the cell to release */
            CellToRelease = CellData->u.KeyList[i];

            /* Check if it's a compressed value name */
            if (KeyValue->Flags & VALUE_COMP_NAME)
            {
                /* Compare compressed names */
                Result = CmpCompareCompressedName(Name,
                                                  KeyValue->Name,
                                                  KeyValue->NameLength);
            }
            else
            {
                /* Compare the Unicode name directly */
                SearchName.Length = KeyValue->NameLength;
                SearchName.MaximumLength = SearchName.Length;
                SearchName.Buffer = KeyValue->Name;
                Result = RtlCompareUnicodeString(Name, &SearchName, TRUE);
            }

            /* Check if we found it */
            if (!Result)
            {
                /* We did... return info to caller */
                if (ChildIndex) *ChildIndex = i;
                *CellIndex = CellData->u.KeyList[i];

                /* Set success state */
                Success = TRUE;
                goto Return;
            }
        }

        /* Got to the end of the list */
        if (ChildIndex) *ChildIndex = i;
        *CellIndex = HCELL_NIL;

        /* Nothing found if we got here */
        Success = TRUE;
        goto Return;
    }

    /* Nothing found... check if the caller wanted more info */
    ASSERT(ChildList->Count == 0);
    if (ChildIndex) *ChildIndex = 0;
    *CellIndex = HCELL_NIL;

    /* Nothing found if we got here */
    return TRUE;

Return:
    /* Release the first cell we got */
    if (CellData) HvReleaseCell(Hive, ChildList->List);

    /* Release the secondary one, if we have one */
    if (CellToRelease) HvReleaseCell(Hive, CellToRelease);
    return Success;
}

HCELL_INDEX
NTAPI
CmpFindValueByName(IN PHHIVE Hive,
                   IN PCM_KEY_NODE KeyNode,
                   IN PUNICODE_STRING Name)
{
    HCELL_INDEX CellIndex;

    /* Call the main function */
    if (!CmpFindNameInList(Hive,
                           &KeyNode->ValueList,
                           Name,
                           NULL,
                           &CellIndex))
    {
        /* Sanity check */
        ASSERT(CellIndex == HCELL_NIL);
    }

    /* Return the index */
    return CellIndex;
}
