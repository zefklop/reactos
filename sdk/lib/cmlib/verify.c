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
 * @name HvpVerifyHiveHeader
 *
 * Internal function to verify that a hive header has valid format.
 */
BOOLEAN CMAPI
HvpVerifyHiveHeader(
    IN PHBASE_BLOCK BaseBlock)
{
    if (BaseBlock->Signature != HV_HBLOCK_SIGNATURE ||
        BaseBlock->Major != HSYS_MAJOR ||
        BaseBlock->Minor < HSYS_MINOR ||
        BaseBlock->Type != HFILE_TYPE_PRIMARY ||
        BaseBlock->Format != HBASE_FORMAT_MEMORY ||
        BaseBlock->Cluster != 1 ||
        BaseBlock->Sequence1 != BaseBlock->Sequence2 ||
        HvpHiveHeaderChecksum(BaseBlock) != BaseBlock->CheckSum)
    {
        DPRINT1("Verify Hive Header failed:\n");
        DPRINT1("    Signature: 0x%x, expected 0x%x; Major: 0x%x, expected 0x%x\n",
                BaseBlock->Signature, HV_HBLOCK_SIGNATURE, BaseBlock->Major, HSYS_MAJOR);
        DPRINT1("    Minor: 0x%x expected to be >= 0x%x; Type: 0x%x, expected 0x%x\n",
                BaseBlock->Minor, HSYS_MINOR, BaseBlock->Type, HFILE_TYPE_PRIMARY);
        DPRINT1("    Format: 0x%x, expected 0x%x; Cluster: 0x%x, expected 1\n",
                BaseBlock->Format, HBASE_FORMAT_MEMORY, BaseBlock->Cluster);
        DPRINT1("    Sequence: 0x%x, expected 0x%x; Checksum: 0x%x, expected 0x%x\n",
                BaseBlock->Sequence1, BaseBlock->Sequence2,
                HvpHiveHeaderChecksum(BaseBlock), BaseBlock->CheckSum);

        return FALSE;
    }

    return TRUE;
}
