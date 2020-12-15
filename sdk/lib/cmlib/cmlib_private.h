
#pragma once

#include "cmlib.h"

#define INVALID_INDEX   0x80000000

NTSTATUS CMAPI
HvpCreateHive(
    IN OUT PHHIVE RegistryHive,
    IN PCUNICODE_STRING FileName OPTIONAL);

NTSTATUS CMAPI
HvpInitializeMemoryHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase,
    IN PCUNICODE_STRING FileName OPTIONAL);

NTSTATUS CMAPI
HvpInitializeFlatHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase);

NTSTATUS CMAPI
HvLoadHive(IN PHHIVE Hive,
           IN PCUNICODE_STRING FileName OPTIONAL);

BOOLEAN CMAPI
HvpVerifyHiveHeader(
    IN PHBASE_BLOCK BaseBlock);

VOID CMAPI
HvpFreeHiveBins(
    PHHIVE Hive);

ULONG
CMAPI
HvpComputeFreeListIndex(
    ULONG Size);

ULONG
NTAPI
CmpFindSubKeyInRoot(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey);

ULONG
NTAPI
CmpFindSubKeyInLeaf(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey);

LONG
NTAPI
CmpCompareInIndex(IN PHHIVE Hive,
                  IN PCUNICODE_STRING SearchName,
                  IN ULONG Count,
                  IN PCM_KEY_INDEX Index,
                  IN PHCELL_INDEX SubKey);

LONG
CMAPI
CmpDoCompareKeyName(IN PHHIVE Hive,
                    IN PCUNICODE_STRING SearchName,
                    IN HCELL_INDEX Cell);

LONG
NTAPI
CmpCompareCompressedName(IN PCUNICODE_STRING SearchName,
                         IN PWCHAR CompressedName,
                         IN ULONG NameLength);

ULONG
NTAPI
CmpComputeHashKey(IN ULONG Hash,
                  IN PCUNICODE_STRING Name,
                  IN BOOLEAN AllowSeparators);

/**
 * @name HvpAllocBaseBlockAligned
 *
 * Internal helper function to allocate cluster-aligned hive base blocks.
 */
static __inline
PHBASE_BLOCK
HvpAllocBaseBlockAligned(
    IN PHHIVE Hive,
    IN BOOLEAN Paged,
    IN ULONG Tag)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Alignment;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Allocate the buffer */
    BaseBlock = Hive->Allocate(Hive->BaseBlockAlloc, Paged, Tag);
    if (!BaseBlock) return NULL;

    /* Check for, and enforce, alignment */
    Alignment = Hive->Cluster * HSECTOR_SIZE -1;
    if ((ULONG_PTR)BaseBlock & Alignment)
    {
        /* Free the old header and reallocate a new one, always paged */
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        BaseBlock = Hive->Allocate(PAGE_SIZE, TRUE, Tag);
        if (!BaseBlock) return NULL;

        Hive->BaseBlockAlloc = PAGE_SIZE;
    }

    return BaseBlock;
}

/**
 * @name HvpInitFileName
 *
 * Internal function to initialize the UNICODE NULL-terminated hive file name
 * member of a hive header by copying the last 31 characters of the file name.
 * Mainly used for debugging purposes.
 */
static
__inline
VOID
HvpInitFileName(
    IN OUT PHBASE_BLOCK BaseBlock,
    IN PCUNICODE_STRING FileName OPTIONAL)
{
    ULONG_PTR Offset;
    SIZE_T    Length;

    /* Always NULL-initialize */
    RtlZeroMemory(BaseBlock->FileName, (HIVE_FILENAME_MAXLEN + 1) * sizeof(WCHAR));

    /* Copy the 31 last characters of the hive file name if any */
    if (!FileName) return;

    if (FileName->Length / sizeof(WCHAR) <= HIVE_FILENAME_MAXLEN)
    {
        Offset = 0;
        Length = FileName->Length;
    }
    else
    {
        Offset = FileName->Length / sizeof(WCHAR) - HIVE_FILENAME_MAXLEN;
        Length = HIVE_FILENAME_MAXLEN * sizeof(WCHAR);
    }

    RtlCopyMemory(BaseBlock->FileName, FileName->Buffer + Offset, Length);
}

static
__inline
PHCELL CMAPI
HvpGetCellHeader(
    PHHIVE RegistryHive,
    HCELL_INDEX CellIndex)
{
    PVOID Block;

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, CellIndex %08lx\n",
             __FUNCTION__, RegistryHive, CellIndex);

    ASSERT(CellIndex != HCELL_NIL);
    if (!RegistryHive->Flat)
    {
        ULONG CellType   = HvGetCellType(CellIndex);
        ULONG CellBlock  = HvGetCellBlock(CellIndex);
        ULONG CellOffset = (CellIndex & HCELL_OFFSET_MASK) >> HCELL_OFFSET_SHIFT;

        ASSERT(CellBlock < RegistryHive->Storage[CellType].Length);
        Block = (PVOID)RegistryHive->Storage[CellType].BlockList[CellBlock].BlockAddress;
        ASSERT(Block != NULL);
        return (PVOID)((ULONG_PTR)Block + CellOffset);
    }
    else
    {
        ASSERT(HvGetCellType(CellIndex) == Stable);
        return (PVOID)((ULONG_PTR)RegistryHive->BaseBlock + HBLOCK_SIZE +
                       CellIndex);
    }
}

static
__inline
NTSTATUS
HvpAddFree(
    PHHIVE RegistryHive,
    PHCELL FreeBlock,
    HCELL_INDEX FreeIndex)
{
    PHCELL_INDEX FreeBlockData;
    HSTORAGE_TYPE Storage;
    ULONG Index;

    ASSERT(RegistryHive != NULL);
    ASSERT(FreeBlock != NULL);

    Storage = HvGetCellType(FreeIndex);
    Index = HvpComputeFreeListIndex((ULONG)FreeBlock->Size);

    FreeBlockData = (PHCELL_INDEX)(FreeBlock + 1);
    *FreeBlockData = RegistryHive->Storage[Storage].FreeDisplay[Index];
    RegistryHive->Storage[Storage].FreeDisplay[Index] = FreeIndex;

    /* FIXME: Eventually get rid of free bins. */

    return STATUS_SUCCESS;
}

