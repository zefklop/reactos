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

/*
 * NOTE: This function should support big values, contrary to CmpValueToData.
 */
BOOLEAN
NTAPI
CmpGetValueData(IN PHHIVE Hive,
                IN PCM_KEY_VALUE Value,
                OUT PULONG Length,
                OUT PVOID *Buffer,
                OUT PBOOLEAN BufferAllocated,
                OUT PHCELL_INDEX CellToRelease)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Value->Signature == CM_KEY_VALUE_SIGNATURE);

    /* Set failure defaults */
    *BufferAllocated = FALSE;
    *Buffer = NULL;
    *CellToRelease = HCELL_NIL;

    /* Check if this is a small key */
    if (CmpIsKeyValueSmall(Length, Value->DataLength))
    {
        /* Return the data immediately */
        *Buffer = &Value->Data;
        return TRUE;
    }

    /* Unsupported at the moment */
    ASSERT_VALUE_BIG(Hive, *Length);

    /* Get the data from the cell */
    *Buffer = HvGetCell(Hive, Value->Data);
    if (!(*Buffer)) return FALSE;

    /* Return success and the cell to be released */
    *CellToRelease = Value->Data;
    return TRUE;
}

/*
 * NOTE: This function doesn't support big values, contrary to CmpGetValueData.
 */
PCELL_DATA
NTAPI
CmpValueToData(IN PHHIVE Hive,
               IN PCM_KEY_VALUE Value,
               OUT PULONG Length)
{
    PCELL_DATA Buffer;
    BOOLEAN BufferAllocated;
    HCELL_INDEX CellToRelease;
    PAGED_CODE();

    /* Sanity check */
    ASSERT(Hive->ReleaseCellRoutine == NULL);

    /* Get the actual data */
    if (!CmpGetValueData(Hive,
                         Value,
                         Length,
                         (PVOID*)&Buffer,
                         &BufferAllocated,
                         &CellToRelease))
    {
        /* We failed */
        ASSERT(BufferAllocated == FALSE);
        ASSERT(Buffer == NULL);
        return NULL;
    }

    /* This should never happen! */
    if (BufferAllocated)
    {
        /* Free the buffer and bugcheck */
        CmpFree(Buffer, 0);
        KeBugCheckEx(REGISTRY_ERROR, 8, 0, (ULONG_PTR)Hive, (ULONG_PTR)Value);
    }

    /* Otherwise, return the cell data */
    return Buffer;
}
