/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"

#include "cmlib_private.h"

#define NDEBUG
#include <debug.h>

/**
 * @name HvInitialize
 *
 * Allocate a new hive descriptor structure and intialize it.
 *
 * @param RegistryHive
 *        Output variable to store pointer to the hive descriptor.
 * @param OperationType
 *        - HV_OPERATION_CREATE_HIVE
 *          Create a new hive for read/write access.
 *        - HV_OPERATION_MEMORY
 *          Load and copy in-memory hive for read/write access. The
 *          pointer to data passed to this routine can be freed after
 *          the function is executed.
 *        - HV_OPERATION_MEMORY_INPLACE
 *          Load an in-memory hive for read-only access. The pointer
 *          to data passed to this routine MUSTN'T be freed until
 *          HvFree is called.
 * @param ChunkBase
 *        Pointer to hive data.
 * @param ChunkSize
 *        Size of passed hive data.
 *
 * @return
 *    STATUS_NO_MEMORY - A memory allocation failed.
 *    STATUS_REGISTRY_CORRUPT - Registry corruption was detected.
 *    STATUS_SUCCESS
 *
 * @see HvFree
 */
NTSTATUS CMAPI
HvInitialize(
    PHHIVE RegistryHive,
    ULONG OperationType,
    ULONG HiveFlags,
    ULONG FileType,
    PVOID HiveData OPTIONAL,
    PALLOCATE_ROUTINE Allocate,
    PFREE_ROUTINE Free,
    PFILE_SET_SIZE_ROUTINE FileSetSize,
    PFILE_WRITE_ROUTINE FileWrite,
    PFILE_READ_ROUTINE FileRead,
    PFILE_FLUSH_ROUTINE FileFlush,
    ULONG Cluster OPTIONAL,
    PCUNICODE_STRING FileName OPTIONAL)
{
    NTSTATUS Status;
    PHHIVE Hive = RegistryHive;

    /*
     * Create a new hive structure that will hold all the maintenance data.
     */

    RtlZeroMemory(Hive, sizeof(HHIVE));
    Hive->Signature = HV_HHIVE_SIGNATURE;

    Hive->Allocate = Allocate;
    Hive->Free = Free;
    Hive->FileSetSize = FileSetSize;
    Hive->FileWrite = FileWrite;
    Hive->FileRead = FileRead;
    Hive->FileFlush = FileFlush;

    Hive->RefreshCount = 0;
    Hive->StorageTypeCount = HTYPE_COUNT;
    Hive->Cluster = Cluster;
    Hive->BaseBlockAlloc = sizeof(HBASE_BLOCK); // == HBLOCK_SIZE

    Hive->Version = HSYS_MINOR;
#if (NTDDI_VERSION < NTDDI_VISTA)
    Hive->Log = (FileType == HFILE_TYPE_LOG);
#endif
    Hive->HiveFlags = HiveFlags & ~HIVE_NOLAZYFLUSH;

    switch (OperationType)
    {
        case HINIT_CREATE:
            Status = HvpCreateHive(Hive, FileName);
            break;

        case HINIT_MEMORY:
            Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
            break;

        case HINIT_FLAT:
            Status = HvpInitializeFlatHive(Hive, HiveData);
            break;

        case HINIT_FILE:
        {
            Status = HvLoadHive(Hive, FileName);
            if ((Status != STATUS_SUCCESS) &&
                (Status != STATUS_REGISTRY_RECOVERED))
            {
                /* Unrecoverable failure */
                return Status;
            }

            /* Check for previous damage */
            ASSERT(Status != STATUS_REGISTRY_RECOVERED);
            break;
        }

        case HINIT_MEMORY_INPLACE:
            // Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
            // break;

        case HINIT_MAPFILE:

        default:
        /* FIXME: A better return status value is needed */
        Status = STATUS_NOT_IMPLEMENTED;
        ASSERT(FALSE);
    }

    if (!NT_SUCCESS(Status)) return Status;

    /* HACK: ROS: Init root key cell and prepare the hive */
    // r31253
    // if (OperationType == HINIT_CREATE) CmCreateRootNode(Hive, L"");
    if (OperationType != HINIT_CREATE) CmPrepareHive(Hive);

    return Status;
}
