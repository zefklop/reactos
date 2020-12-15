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
 * @name HvpInitializeMemoryHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * an existing hive stored in memory. The data of the hive is copied
 * and it is prepared for read/write access.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpInitializeMemoryHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase,
    IN PCUNICODE_STRING FileName OPTIONAL)
{
    SIZE_T BlockIndex;
    PHBIN Bin, NewBin;
    ULONG i;
    ULONG BitmapSize;
    PULONG BitmapBuffer;
    SIZE_T ChunkSize;

    ChunkSize = ChunkBase->Length;
    DPRINT("ChunkSize: %zx\n", ChunkSize);

    if (ChunkSize < sizeof(HBASE_BLOCK) ||
        !HvpVerifyHiveHeader(ChunkBase))
    {
        DPRINT1("Registry is corrupt: ChunkSize 0x%zx < sizeof(HBASE_BLOCK) 0x%zx, "
                "or HvpVerifyHiveHeader() failed\n", ChunkSize, sizeof(HBASE_BLOCK));
        return STATUS_REGISTRY_CORRUPT;
    }

    /* Allocate the base block */
    Hive->BaseBlock = HvpAllocBaseBlockAligned(Hive, FALSE, TAG_CM);
    if (Hive->BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Hive->BaseBlock, ChunkBase, sizeof(HBASE_BLOCK));

    /* Setup hive data */
    Hive->Version = ChunkBase->Minor;

    /*
     * Build a block list from the in-memory chunk and copy the data as
     * we go.
     */

    Hive->Storage[Stable].Length = (ULONG)(ChunkSize / HBLOCK_SIZE);
    Hive->Storage[Stable].BlockList =
        Hive->Allocate(Hive->Storage[Stable].Length *
                       sizeof(HMAP_ENTRY), FALSE, TAG_CM);
    if (Hive->Storage[Stable].BlockList == NULL)
    {
        DPRINT1("Allocating block list failed\n");
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    for (BlockIndex = 0; BlockIndex < Hive->Storage[Stable].Length; )
    {
        Bin = (PHBIN)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HBLOCK_SIZE);
        if (Bin->Signature != HV_HBIN_SIGNATURE ||
           (Bin->Size % HBLOCK_SIZE) != 0)
        {
            DPRINT1("Invalid bin at BlockIndex %lu, Signature 0x%x, Size 0x%x\n",
                    (unsigned long)BlockIndex, (unsigned)Bin->Signature, (unsigned)Bin->Size);
            Hive->Free(Hive->Storage[Stable].BlockList, 0);
            Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
            return STATUS_REGISTRY_CORRUPT;
        }

        NewBin = Hive->Allocate(Bin->Size, TRUE, TAG_CM);
        if (NewBin == NULL)
        {
            Hive->Free(Hive->Storage[Stable].BlockList, 0);
            Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
            return STATUS_NO_MEMORY;
        }

        Hive->Storage[Stable].BlockList[BlockIndex].BinAddress = (ULONG_PTR)NewBin;
        Hive->Storage[Stable].BlockList[BlockIndex].BlockAddress = (ULONG_PTR)NewBin;

        RtlCopyMemory(NewBin, Bin, Bin->Size);

        if (Bin->Size > HBLOCK_SIZE)
        {
            for (i = 1; i < Bin->Size / HBLOCK_SIZE; i++)
            {
                Hive->Storage[Stable].BlockList[BlockIndex + i].BinAddress = (ULONG_PTR)NewBin;
                Hive->Storage[Stable].BlockList[BlockIndex + i].BlockAddress =
                    ((ULONG_PTR)NewBin + (i * HBLOCK_SIZE));
            }
        }

        BlockIndex += Bin->Size / HBLOCK_SIZE;
    }

    if (HvpCreateHiveFreeCellList(Hive))
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    BitmapSize = ROUND_UP(Hive->Storage[Stable].Length,
                          sizeof(ULONG) * 8) / 8;
    BitmapBuffer = (PULONG)Hive->Allocate(BitmapSize, TRUE, TAG_CM);
    if (BitmapBuffer == NULL)
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
    RtlClearAllBits(&Hive->DirtyVector);

    HvpInitFileName(Hive->BaseBlock, FileName);

    return STATUS_SUCCESS;
}
