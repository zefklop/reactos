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
 * @name HvFree
 *
 * Free all stroage and handles associated with hive descriptor.
 * But do not free the hive descriptor itself.
 */
VOID CMAPI
HvFree(
    PHHIVE RegistryHive)
{
    if (!RegistryHive->ReadOnly)
    {
        /* Release hive bitmap */
        if (RegistryHive->DirtyVector.Buffer)
        {
            RegistryHive->Free(RegistryHive->DirtyVector.Buffer, 0);
        }

        HvpFreeHiveBins(RegistryHive);

        /* Free the BaseBlock */
        if (RegistryHive->BaseBlock)
        {
            RegistryHive->Free(RegistryHive->BaseBlock, RegistryHive->BaseBlockAlloc);
            RegistryHive->BaseBlock = NULL;
        }
    }
}

/* EOF */
