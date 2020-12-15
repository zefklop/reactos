/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib_private.h"

#define NDEBUG
#include <debug.h>

/**
 * @name HvpFreeHiveBins
 *
 * Internal function to free all bin storage associated with a hive descriptor.
 */
VOID CMAPI
HvpFreeHiveBins(
    PHHIVE Hive)
{
    ULONG i;
    PHBIN Bin;
    ULONG Storage;

    for (Storage = 0; Storage < Hive->StorageTypeCount; Storage++)
    {
        Bin = NULL;
        for (i = 0; i < Hive->Storage[Storage].Length; i++)
        {
            if (Hive->Storage[Storage].BlockList[i].BinAddress == (ULONG_PTR)NULL)
                continue;
            if (Hive->Storage[Storage].BlockList[i].BinAddress != (ULONG_PTR)Bin)
            {
                Bin = (PHBIN)Hive->Storage[Storage].BlockList[i].BinAddress;
                Hive->Free((PHBIN)Hive->Storage[Storage].BlockList[i].BinAddress, 0);
            }
            Hive->Storage[Storage].BlockList[i].BinAddress = (ULONG_PTR)NULL;
            Hive->Storage[Storage].BlockList[i].BlockAddress = (ULONG_PTR)NULL;
        }

        if (Hive->Storage[Storage].Length)
            Hive->Free(Hive->Storage[Storage].BlockList, 0);
    }
}
