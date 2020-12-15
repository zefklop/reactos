/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib_private.h"

#define NDEBUG
#include <debug.h>

ULONG
CMAPI
HvpComputeFreeListIndex(
    ULONG Size)
{
    ULONG Index;
    static CCHAR FindFirstSet[128] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

    ASSERT(Size >= (1 << 3));
    Index = (Size >> 3) - 1;
    if (Index >= 16)
    {
        if (Index > 127)
            Index = 23;
        else
            Index = FindFirstSet[Index] + 16;
    }

    return Index;
}

NTSTATUS CMAPI
HvpCreateHiveFreeCellList(
    PHHIVE Hive)
{
    HCELL_INDEX BlockOffset;
    PHCELL FreeBlock;
    ULONG BlockIndex;
    ULONG FreeOffset;
    PHBIN Bin;
    NTSTATUS Status;
    ULONG Index;

    /* Initialize the free cell list */
    for (Index = 0; Index < 24; Index++)
    {
        Hive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
        Hive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
    }

    BlockOffset = 0;
    BlockIndex = 0;
    while (BlockIndex < Hive->Storage[Stable].Length)
    {
        Bin = (PHBIN)Hive->Storage[Stable].BlockList[BlockIndex].BinAddress;

        /* Search free blocks and add to list */
        FreeOffset = sizeof(HBIN);
        while (FreeOffset < Bin->Size)
        {
            FreeBlock = (PHCELL)((ULONG_PTR)Bin + FreeOffset);
            if (FreeBlock->Size > 0)
            {
                Status = HvpAddFree(Hive, FreeBlock, Bin->FileOffset + FreeOffset);
                if (!NT_SUCCESS(Status))
                    return Status;

                FreeOffset += FreeBlock->Size;
            }
            else
            {
                FreeOffset -= FreeBlock->Size;
            }
        }

        BlockIndex += Bin->Size / HBLOCK_SIZE;
        BlockOffset += Bin->Size;
    }

    return STATUS_SUCCESS;
}
