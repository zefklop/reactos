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
 * @name HvpInitializeFlatHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * a hive stored in memory. The in-memory data of the hive are directly
 * used and it is read-only accessible.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpInitializeFlatHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase)
{
    if (!HvpVerifyHiveHeader(ChunkBase))
        return STATUS_REGISTRY_CORRUPT;

    /* Setup hive data */
    Hive->BaseBlock = ChunkBase;
    Hive->Version = ChunkBase->Minor;
    Hive->Flat = TRUE;
    Hive->ReadOnly = TRUE;

    Hive->StorageTypeCount = 1;

    /* Set default boot type */
    ChunkBase->BootType = 0;

    return STATUS_SUCCESS;
}
