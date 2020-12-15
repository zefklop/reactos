/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib_private.h"

#define NDEBUG
#include <debug.h>

typedef enum _RESULT
{
    NotHive,
    Fail,
    NoMemory,
    HiveSuccess,
    RecoverHeader,
    RecoverData,
    SelfHeal
} RESULT;

static
RESULT CMAPI
HvpGetHiveHeader(IN PHHIVE Hive,
                 IN PHBASE_BLOCK *HiveBaseBlock,
                 IN PLARGE_INTEGER TimeStamp)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Result;
    ULONG Offset = 0;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Assume failure and allocate the base block */
    *HiveBaseBlock = NULL;
    BaseBlock = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!BaseBlock) return NoMemory;

    /* Clear it */
    RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

    /* Now read it from disk */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            BaseBlock,
                            Hive->Cluster * HSECTOR_SIZE);

    /* Couldn't read: assume it's not a hive */
    if (!Result) return NotHive;

    /* Do validation */
    if (!HvpVerifyHiveHeader(BaseBlock)) return NotHive;

    /* Return information */
    *HiveBaseBlock = BaseBlock;
    *TimeStamp = BaseBlock->TimeStamp;
    return HiveSuccess;
}

NTSTATUS CMAPI
HvLoadHive(IN PHHIVE Hive,
           IN PCUNICODE_STRING FileName OPTIONAL)
{
    NTSTATUS Status;
    PHBASE_BLOCK BaseBlock = NULL;
    ULONG Result;
    LARGE_INTEGER TimeStamp;
    ULONG Offset = 0;
    PVOID HiveData;
    ULONG FileSize;

    /* Get the hive header */
    Result = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);
    switch (Result)
    {
        /* Out of memory */
        case NoMemory:

            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;

        /* Not a hive */
        case NotHive:

            /* Fail */
            return STATUS_NOT_REGISTRY_FILE;

        /* Has recovery data */
        case RecoverData:
        case RecoverHeader:

            /* Fail */
            return STATUS_REGISTRY_CORRUPT;
    }

    /* Set default boot type */
    BaseBlock->BootType = 0;

    /* Setup hive data */
    Hive->BaseBlock = BaseBlock;
    Hive->Version = BaseBlock->Minor;

    /* Allocate a buffer large enough to hold the hive */
    FileSize = HBLOCK_SIZE + BaseBlock->Length; // == sizeof(HBASE_BLOCK) + BaseBlock->Length;
    HiveData = Hive->Allocate(FileSize, TRUE, TAG_CM);
    if (!HiveData)
    {
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Now read the whole hive */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            HiveData,
                            FileSize);
    if (!Result)
    {
        Hive->Free(HiveData, FileSize);
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NOT_REGISTRY_FILE;
    }

    // This is a HACK!
    /* Free our base block... it's usless in this implementation */
    Hive->Free(BaseBlock, Hive->BaseBlockAlloc);

    /* Initialize the hive directly from memory */
    Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
    if (!NT_SUCCESS(Status))
        Hive->Free(HiveData, FileSize);

    return Status;
}
