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
 * @name HvpCreateHive
 *
 * Internal helper function to initialize a hive descriptor structure
 * for a newly created hive in memory.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpCreateHive(
    IN OUT PHHIVE RegistryHive,
    IN PCUNICODE_STRING FileName OPTIONAL)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Index;

    /* Allocate the base block */
    BaseBlock = HvpAllocBaseBlockAligned(RegistryHive, FALSE, TAG_CM);
    if (BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    /* Clear it */
    RtlZeroMemory(BaseBlock, RegistryHive->BaseBlockAlloc);

    BaseBlock->Signature = HV_HBLOCK_SIGNATURE;
    BaseBlock->Major = HSYS_MAJOR;
    BaseBlock->Minor = HSYS_MINOR;
    BaseBlock->Type = HFILE_TYPE_PRIMARY;
    BaseBlock->Format = HBASE_FORMAT_MEMORY;
    BaseBlock->Cluster = 1;
    BaseBlock->RootCell = HCELL_NIL;
    BaseBlock->Length = 0;
    BaseBlock->Sequence1 = 1;
    BaseBlock->Sequence2 = 1;
    BaseBlock->TimeStamp.QuadPart = 0ULL;

    /*
     * No need to compute the checksum since
     * the hive resides only in memory so far.
     */
    BaseBlock->CheckSum = 0;

    /* Set default boot type */
    BaseBlock->BootType = 0;

    /* Setup hive data */
    RegistryHive->BaseBlock = BaseBlock;
    RegistryHive->Version = BaseBlock->Minor; // == HSYS_MINOR

    for (Index = 0; Index < 24; Index++)
    {
        RegistryHive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
        RegistryHive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
    }

    HvpInitFileName(BaseBlock, FileName);

    return STATUS_SUCCESS;
}
