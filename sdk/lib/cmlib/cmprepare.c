/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib_private.h"

#define NDEBUG
#include <debug.h>

static VOID CMAPI
CmpPrepareKey(
    PHHIVE RegistryHive,
    PCM_KEY_NODE KeyCell);

static VOID CMAPI
CmpPrepareIndexOfKeys(
    PHHIVE RegistryHive,
    PCM_KEY_INDEX IndexCell)
{
    ULONG i;

    if (IndexCell->Signature == CM_KEY_INDEX_ROOT ||
        IndexCell->Signature == CM_KEY_INDEX_LEAF)
    {
        for (i = 0; i < IndexCell->Count; i++)
        {
            PCM_KEY_INDEX SubIndexCell = HvGetCell(RegistryHive, IndexCell->List[i]);
            if (SubIndexCell->Signature == CM_KEY_NODE_SIGNATURE)
                CmpPrepareKey(RegistryHive, (PCM_KEY_NODE)SubIndexCell);
            else
                CmpPrepareIndexOfKeys(RegistryHive, SubIndexCell);
        }
   }
    else if (IndexCell->Signature == CM_KEY_FAST_LEAF ||
             IndexCell->Signature == CM_KEY_HASH_LEAF)
    {
        PCM_KEY_FAST_INDEX HashCell = (PCM_KEY_FAST_INDEX)IndexCell;
        for (i = 0; i < HashCell->Count; i++)
        {
            PCM_KEY_NODE SubKeyCell = HvGetCell(RegistryHive, HashCell->List[i].Cell);
            CmpPrepareKey(RegistryHive, SubKeyCell);
        }
    }
    else
    {
        DPRINT1("IndexCell->Signature %x\n", IndexCell->Signature);
        ASSERT(FALSE);
    }
}

static VOID CMAPI
CmpPrepareKey(
    PHHIVE RegistryHive,
    PCM_KEY_NODE KeyCell)
{
    PCM_KEY_INDEX IndexCell;

    ASSERT(KeyCell->Signature == CM_KEY_NODE_SIGNATURE);

    KeyCell->SubKeyCounts[Volatile] = 0;
    // KeyCell->SubKeyLists[Volatile] = HCELL_NIL; // FIXME! Done only on Windows < XP.

    /* Enumerate and add subkeys */
    if (KeyCell->SubKeyCounts[Stable] > 0)
    {
        IndexCell = HvGetCell(RegistryHive, KeyCell->SubKeyLists[Stable]);
        CmpPrepareIndexOfKeys(RegistryHive, IndexCell);
    }
}

VOID CMAPI
CmPrepareHive(
    PHHIVE RegistryHive)
{
    PCM_KEY_NODE RootCell;

    RootCell = HvGetCell(RegistryHive, RegistryHive->BaseBlock->RootCell);
    CmpPrepareKey(RegistryHive, RootCell);
}
