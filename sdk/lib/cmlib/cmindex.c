/*
 * PROJECT:         ReactOS Registry Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/cmlib/cmindex.c
 * PURPOSE:         Configuration Manager - Cell Indexes
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "cmlib_private.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define CmpMaxFastIndexPerHblock                        \
    ((HBLOCK_SIZE - (sizeof(HBIN) + sizeof(HCELL) +     \
                     FIELD_OFFSET(CM_KEY_FAST_INDEX, List))) / sizeof(CM_INDEX))

#define CmpMaxIndexPerHblock                            \
    ((HBLOCK_SIZE - (sizeof(HBIN) + sizeof(HCELL) +     \
                     FIELD_OFFSET(CM_KEY_INDEX, List))) / sizeof(HCELL_INDEX) - 1)

/* FUNCTIONS *****************************************************************/

BOOLEAN
NTAPI
CmpMarkIndexDirty(IN PHHIVE Hive,
                  IN HCELL_INDEX ParentKey,
                  IN HCELL_INDEX TargetKey)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING SearchName;
    BOOLEAN IsCompressed;
    ULONG i, Result;
    PCM_KEY_INDEX Index;
    HCELL_INDEX IndexCell, Child = HCELL_NIL, CellToRelease = HCELL_NIL;

    /* Get the target key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (!Node) return FALSE;

    /* Check if it's compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Remember this for later */
        IsCompressed = TRUE;

        /* Build the search name */
        SearchName.Length = CmpCompressedNameSize(Node->Name,
                                                  Node->NameLength);
        SearchName.MaximumLength = SearchName.Length;
        SearchName.Buffer = CmpAllocate(SearchName.Length,
                                        TRUE,
                                        TAG_CM);
        if (!SearchName.Buffer)
        {
            /* Fail */
            HvReleaseCell(Hive, TargetKey);
            return FALSE;
        }

        /* Copy it */
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              Node->Name,
                              Node->NameLength);
    }
    else
    {
        /* Name isn't compressed, build it directly from the node */
        IsCompressed = FALSE;
        SearchName.Length = Node->NameLength;
        SearchName.MaximumLength = Node->NameLength;
        SearchName.Buffer = Node->Name;
    }

    /* We can release the target key now */
    HvReleaseCell(Hive, TargetKey);

    /* Now get the parent key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if (!Node) goto Quickie;

    /* Loop all hive storage */
    for (i = 0; i < Hive->StorageTypeCount; i++)
    {
        /* Check if any subkeys are in this index */
        if (Node->SubKeyCounts[i])
        {
            /* Get the cell index */
            //ASSERT(HvIsCellAllocated(Hive, Node->SubKeyLists[i]));
            IndexCell = Node->SubKeyLists[i];

            /* Check if we had anything to release from before */
            if (CellToRelease != HCELL_NIL)
            {
                /* Release it now */
                HvReleaseCell(Hive, CellToRelease);
                CellToRelease = HCELL_NIL;
            }

            /* Get the key index for the cell */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
            if (!Index) goto Quickie;

            /* Release it at the next iteration or below */
            CellToRelease = IndexCell;

            /* Check if this is a root */
            if (Index->Signature == CM_KEY_INDEX_ROOT)
            {
                /* Get the child inside the root */
                Result = CmpFindSubKeyInRoot(Hive, Index, &SearchName, &Child);
                if (Result & INVALID_INDEX) goto Quickie;
                if (Child == HCELL_NIL) continue;

                /* We found it, mark the cell dirty */
                HvMarkCellDirty(Hive, IndexCell, FALSE);

                /* Check if we had anything to release from before */
                if (CellToRelease != HCELL_NIL)
                {
                    /* Release it now */
                    HvReleaseCell(Hive, CellToRelease);
                    CellToRelease = HCELL_NIL;
                }

                /* Now this child is the index, get the actual key index */
                IndexCell = Child;
                Index = (PCM_KEY_INDEX)HvGetCell(Hive, Child);
                if (!Index) goto Quickie;

                /* Release it later */
                CellToRelease = Child;
            }

            /* Make sure this is a valid index */
            ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
                   (Index->Signature == CM_KEY_FAST_LEAF) ||
                   (Index->Signature == CM_KEY_HASH_LEAF));

            /* Find the child in the leaf */
            Result = CmpFindSubKeyInLeaf(Hive, Index, &SearchName, &Child);
            if (Result & INVALID_INDEX) goto Quickie;
            if (Child != HCELL_NIL)
            {
                /* We found it, free the name now */
                if (IsCompressed) CmpFree(SearchName.Buffer, 0);

                /* Release the parent key */
                HvReleaseCell(Hive, ParentKey);

                /* Check if we had a left over cell to release */
                if (CellToRelease != HCELL_NIL)
                {
                    /* Release it */
                    HvReleaseCell(Hive, CellToRelease);
                }

                /* And mark the index cell dirty */
                HvMarkCellDirty(Hive, IndexCell, FALSE);
                return TRUE;
            }
        }
    }

Quickie:
    /* Release any cells that we still hold */
    if (Node) HvReleaseCell(Hive, ParentKey);
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);

    /* Free the search name and return failure */
    if (IsCompressed) CmpFree(SearchName.Buffer, 0);
    return FALSE;
}

HCELL_INDEX
NTAPI
CmpAddToLeaf(IN PHHIVE Hive,
             IN HCELL_INDEX LeafCell,
             IN HCELL_INDEX NewKey,
             IN PUNICODE_STRING Name)
{
    PCM_KEY_INDEX Leaf;
    PCM_KEY_FAST_INDEX FastLeaf;
    ULONG Size, OldSize, EntrySize, i, j;
    HCELL_INDEX NewCell, Child;
    LONG Result;

    /* Mark the leaf dirty */
    HvMarkCellDirty(Hive, LeafCell, FALSE);

    /* Get the leaf cell */
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
        return HCELL_NIL;
    }

    /* Release it */
    HvReleaseCell(Hive, LeafCell);

    /* Check if this is an index leaf */
    if (Leaf->Signature == CM_KEY_INDEX_LEAF)
    {
        /* This is an old-style leaf */
        FastLeaf = NULL;
        EntrySize = sizeof(HCELL_INDEX);
    }
    else
    {
        /* Sanity check */
        ASSERT((Leaf->Signature == CM_KEY_FAST_LEAF) ||
               (Leaf->Signature == CM_KEY_HASH_LEAF));

        /* This is a new-style optimized fast (or hash) leaf */
        FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
        EntrySize = sizeof(CM_INDEX);
    }

    /* Get the current size of the leaf */
    OldSize = HvGetCellSize(Hive, Leaf);

    /* Calculate the size of the free entries */
    Size = OldSize;
    Size -= EntrySize * Leaf->Count + FIELD_OFFSET(CM_KEY_INDEX, List);

    /* Assume we'll re-use the same leaf */
    NewCell = LeafCell;

    /* Check if we're out of space */
    if ((Size / EntrySize) < 1)
    {
        /* Grow the leaf by 1.5x, making sure we can at least fit this entry */
        Size = OldSize + OldSize / 2;
        if (Size < (OldSize + EntrySize)) Size = OldSize + EntrySize;

        /* Re-allocate the leaf */
        NewCell = HvReallocateCell(Hive, LeafCell, Size);
        if (NewCell == HCELL_NIL) return HCELL_NIL;

        /* Get the leaf cell */
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
        if (!Leaf)
        {
            /* This shouldn't happen */
            ASSERT(FALSE);
            return HCELL_NIL;
        }

        /* Release the cell */
        HvReleaseCell(Hive, NewCell);

        /* Update the fast leaf pointer if we had one */
        if (FastLeaf) FastLeaf = (PCM_KEY_FAST_INDEX)Leaf;
    }

    /* Find the insertion point for our entry */
    i = CmpFindSubKeyInLeaf(Hive, Leaf, Name, &Child);
    if (i & INVALID_INDEX) return HCELL_NIL;
    ASSERT(Child == HCELL_NIL);

    /* Check if we're not last */
    if (i != Leaf->Count)
    {
        /* Find out where we should go */
        Result = CmpCompareInIndex(Hive,
                                   Name,
                                   i,
                                   Leaf,
                                   &Child);
        if (Result == 2) return HCELL_NIL;
        ASSERT(Result != 0);

        /* Check if we come after */
        if (Result > 0)
        {
            /* We do, insert us after the key */
            ASSERT(Result == 1);
            i++;
        }

        /* Check if we're still not last */
        if (i != Leaf->Count)
        {
            /* Check if we had a fast leaf or not */
            if (FastLeaf)
            {
                /* Copy the fast indexes */
                RtlMoveMemory(&FastLeaf->List[i + 1],
                              &FastLeaf->List[i],
                              (FastLeaf->Count - i) * sizeof(CM_INDEX));
            }
            else
            {
                /* Copy the indexes themselves */
                RtlMoveMemory(&Leaf->List[i + 1],
                              &Leaf->List[i],
                              (Leaf->Count - i) * sizeof(HCELL_INDEX));
            }
        }
    }

    /* Check if this is a new-style leaf */
    if (FastLeaf)
    {
        /* Set our cell */
        FastLeaf->List[i].Cell = NewKey;

        /* Check if this is a hash leaf */
        if( FastLeaf->Signature == CM_KEY_HASH_LEAF )
        {
            /* Set our hash key */
            FastLeaf->List[i].HashKey = CmpComputeHashKey(0, Name, FALSE);
        }
        else
        {
            /* First, clear the name */
            FastLeaf->List[i].NameHint[0] = 0;
            FastLeaf->List[i].NameHint[1] = 0;
            FastLeaf->List[i].NameHint[2] = 0;
            FastLeaf->List[i].NameHint[3] = 0;

            /* Now, figure out if we can fit */
            if (Name->Length / sizeof(WCHAR) < 4)
            {
                /* We can fit, use our length */
                j = Name->Length / sizeof(WCHAR);
            }
            else
            {
                /* We can't, use a maximum of 4 */
                j = 4;
            }

            /* Now fill out the name hint */
            do
            {
                /* Look for invalid characters and break out if we found one */
                if ((USHORT)Name->Buffer[j - 1] > (UCHAR)-1) break;

                /* Otherwise, copy the a character */
                FastLeaf->List[i].NameHint[j - 1] = (UCHAR)Name->Buffer[j - 1];
            } while (--j > 0);
        }
    }
    else
    {
        /* This is an old-style leaf, just set our index directly */
        Leaf->List[i] = NewKey;
    }

    /* Update the leaf count and return the new cell */
    Leaf->Count += 1;
    return NewCell;
}

HCELL_INDEX
NTAPI
CmpSplitLeaf(IN PHHIVE Hive,
             IN HCELL_INDEX RootCell,
             IN ULONG RootSelect,
             IN HSTORAGE_TYPE Type)
{
    PCM_KEY_INDEX IndexKey, LeafKey, NewKey;
    PCM_KEY_FAST_INDEX FastLeaf;
    HCELL_INDEX LeafCell, NewCell;
    USHORT FirstHalf, LastHalf;
    ULONG EntrySize, TotalSize;

    /* Get the cell */
    IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);

    /* Check if we've got valid IndexKey */
    if (!IndexKey) return HCELL_NIL;

    /* Get the leaf cell and key */
    LeafCell = IndexKey->List[RootSelect];
    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

    /* Check if we've got valid LeafKey */
    if (!LeafKey) return HCELL_NIL;

    /* We are going to divide this leaf into two halves */
    FirstHalf = (LeafKey->Count / 2);
    LastHalf = LeafKey->Count - FirstHalf;

    /* Now check what kind of hive we're dealing with,
     * and compute entry size
     */
    if (Hive->Version >= 5)
    {
        /* XP Hive: Use hash leaf */
        ASSERT(LeafKey->Signature == CM_KEY_HASH_LEAF);
        EntrySize = sizeof(CM_INDEX);
    }
    else
    {
        /* Use index leaf */
        ASSERT(LeafKey->Signature == CM_KEY_INDEX_LEAF);
        EntrySize = sizeof(HCELL_INDEX);
    }

    /* Compute the total size */
    TotalSize = (EntrySize * LastHalf) + FIELD_OFFSET(CM_KEY_INDEX, List) + 1;

    /* Mark the leaf cell dirty */
    HvMarkCellDirty(Hive, LeafCell, FALSE);

    /* Make sure its type is the same */
    ASSERT(HvGetCellType(LeafCell) == Type);

    /* Allocate the cell, fail in case of error */
    NewCell = HvAllocateCell(Hive, TotalSize, Type, LeafCell);
    if (NewCell == HCELL_NIL) return NewCell;

    /* Get the key */
    NewKey = (PCM_KEY_INDEX)HvGetCell(Hive, NewCell);
    if (!NewKey)
    {
        /* Free the cell and exit - should not happen! */
        ASSERT(FALSE);
        HvFreeCell(Hive, NewCell);
        return HCELL_NIL;
    }

    /* Release the newly created cell */
    HvReleaseCell(Hive, NewCell);

    /* Set its signature according to the version of the hive */
    if (Hive->Version >= 5)
    {
        /* XP Hive: Use hash leaf signature */
        NewKey->Signature = CM_KEY_HASH_LEAF;
    }
    else
    {
        /* Use index leaf signature */
        NewKey->Signature = CM_KEY_INDEX_LEAF;
    }

    /* Calculate the size of the free entries in the root key */
    TotalSize = HvGetCellSize(Hive, IndexKey) -
        (IndexKey->Count * sizeof(HCELL_INDEX)) -
        FIELD_OFFSET(CM_KEY_INDEX, List);

    /* Check if we're out of space */
    if (TotalSize / sizeof(HCELL_INDEX) < 1)
    {
        /* Grow the leaf by one more entry */
        TotalSize = HvGetCellSize(Hive, IndexKey) + sizeof(HCELL_INDEX);

        /* Re-allocate the root */
        RootCell = HvReallocateCell(Hive, RootCell, TotalSize);
        if (RootCell == HCELL_NIL)
        {
            /* Free the cell and exit */
            HvFreeCell(Hive, NewCell);
            return HCELL_NIL;
        }

        /* Get the leaf cell */
        IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, RootCell);
        if (!IndexKey)
        {
            /* This shouldn't happen */
            ASSERT(FALSE);
            return HCELL_NIL;
        }
    }

    /* Splitting is done, now we need to copy the contents,
     * according to the hive version
     */
    if (Hive->Version >= 5)
    {
        /* Copy the fast indexes */
        FastLeaf = (PCM_KEY_FAST_INDEX)LeafKey;
        RtlMoveMemory(&NewKey->List[0],
                      &FastLeaf->List[FirstHalf],
                      LastHalf * EntrySize);
    }
    else
    {
        /* Copy the indexes themselves */
        RtlMoveMemory(&NewKey->List[0],
                      &LeafKey->List[FirstHalf],
                      LastHalf * EntrySize);
    }

    /* Shift the data inside the root key */
    if ((RootSelect + 1) < IndexKey->Count)
    {
        RtlMoveMemory(&IndexKey->List[RootSelect + 2],
                      &IndexKey->List[RootSelect + 1],
                      (IndexKey->Count -
                      (RootSelect + 1)) * sizeof(HCELL_INDEX));
    }

    /* Make sure both old and new computed counts are valid */
    ASSERT(FirstHalf != 0);
    ASSERT(LastHalf != 0);

    /* Update the count value of old and new keys */
    LeafKey->Count = FirstHalf;
    NewKey->Count = LastHalf;

    /* Increase the count value of the root key */
    IndexKey->Count++;

    /* Set the new cell in root's list */
    IndexKey->List[RootSelect + 1] = NewCell;

    /* Return the root cell */
    return RootCell;
}

HCELL_INDEX
NTAPI
CmpSelectLeaf(IN PHHIVE Hive,
              IN PCM_KEY_NODE KeyNode,
              IN PUNICODE_STRING Name,
              IN HSTORAGE_TYPE Type,
              IN PHCELL_INDEX *RootCell)
{
    PCM_KEY_INDEX IndexKey, LeafKey;
    PCM_KEY_FAST_INDEX FastLeaf;
    HCELL_INDEX LeafCell, CurrentCell;
    ULONG SubKeyIndex;
    LONG Result;

    /* Mark it as dirty */
    HvMarkCellDirty(Hive, KeyNode->SubKeyLists[Type], FALSE);

    /* Get the cell */
    IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);

    /* Check if we've got a valid key */
    if (!IndexKey)
    {
        /* Should not happen! */
        ASSERT(IndexKey != NULL);
        return HCELL_NIL;
    }

    /* Sanity check */
    ASSERT(IndexKey->Signature == CM_KEY_INDEX_ROOT);

    while (TRUE)
    {
        /* Find subkey */
        SubKeyIndex = CmpFindSubKeyInRoot(Hive, IndexKey, Name, &LeafCell);

        /* Make sure we found something valid */
        if (SubKeyIndex & INVALID_INDEX) return HCELL_NIL;

        /* Try to fit it into the LeafCell, if it was found */
        if (LeafCell != HCELL_NIL)
        {
            /* Get the leaf key */
            LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

            /* Check for failure */
            if (!LeafKey) return HCELL_NIL;

            /* Check if it fits into this leaf and break */
            if (LeafKey->Count < CmpMaxIndexPerHblock)
            {
                /* Fill in the result and return it */
                *RootCell = &IndexKey->List[SubKeyIndex];
                return LeafCell;
            }

            /* It didn't fit, so proceed to splitting */
        }
        else
        {
            /* Get the leaf cell at the very end */
            LeafCell = IndexKey->List[SubKeyIndex];
            LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

            /* Return an error in case of problems */
            if (!LeafKey) return HCELL_NIL;

            /* Choose the cell to search from depending on the key type */
            if ((LeafKey->Signature == CM_KEY_FAST_LEAF) ||
                (LeafKey->Signature == CM_KEY_HASH_LEAF))
            {
                FastLeaf = (PCM_KEY_FAST_INDEX)LeafKey;
                CurrentCell = FastLeaf->List[0].Cell;
            }
            else
            {
                /* Make sure it's an index leaf */
                ASSERT(LeafKey->Signature == CM_KEY_INDEX_LEAF);
                CurrentCell = LeafKey->List[0];
            }

            /* Do a name compare */
            Result = CmpDoCompareKeyName(Hive, Name, CurrentCell);

            /* Check for failure */
            if (Result == 2) return HCELL_NIL;

            /* Names can't be equal, ensure that */
            ASSERT(Result != 0);

            /* Check if it's above */
            if (Result >= 0)
            {
                /* Get the cell in the index */
                LeafCell = IndexKey->List[SubKeyIndex];
                LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                /* Return an error in case of problems */
                if (!LeafKey) return HCELL_NIL;

                /* Check if it fits into this leaf */
                if (LeafKey->Count < CmpMaxIndexPerHblock)
                {
                    /* Fill in the result and return the cell */
                    *RootCell = &IndexKey->List[SubKeyIndex];
                    return LeafCell;
                }

                /* No, it doesn't fit, check the next adjacent leaf */
                if ((SubKeyIndex + 1) < IndexKey->Count)
                {
                    /* Yes, there is space */
                    LeafCell = IndexKey->List[SubKeyIndex + 1];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[SubKeyIndex + 1];
                        return LeafCell;
                    }
                }

                /* It didn't fit, so proceed to splitting */
            }
            else
            {
                /* No, it's below, check the subkey index */
                if (SubKeyIndex > 0)
                {
                    /* There should be space at the leaf one before that */
                    LeafCell = IndexKey->List[SubKeyIndex - 1];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[SubKeyIndex - 1];
                        return LeafCell;
                    }
                }
                else
                {
                    /* Use the first leaf, if possible */
                    LeafCell = IndexKey->List[0];
                    LeafKey = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);

                    /* Return an error in case of problems */
                    if (!LeafKey) return HCELL_NIL;

                    /* Check if it fits and break */
                    if (LeafKey->Count < CmpMaxIndexPerHblock)
                    {
                        /* Fill in the result and return the cell */
                        *RootCell = &IndexKey->List[0];
                        return LeafCell;
                    }
                }

                /* It didn't fit into either, so proceed to splitting */
            }
        }

        /* We need to split */
        CurrentCell = CmpSplitLeaf(Hive,
                                   KeyNode->SubKeyLists[Type],
                                   SubKeyIndex,
                                   Type);

        /* Return failure in case splitting failed */
        if (CurrentCell == HCELL_NIL) return CurrentCell;

        /* Set the SubKeyLists value with the new key */
        KeyNode->SubKeyLists[Type] = CurrentCell;

        /* Get the new cell */
        IndexKey = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);

        /* Return in case of failure */
        if (!IndexKey) return HCELL_NIL;

        /* Make sure the new key became index root */
        ASSERT(IndexKey->Signature == CM_KEY_INDEX_ROOT);

        /* Now loop over with the new IndexKey value, which definately
         * has the space now
         */
    }

    /* Shouldn't come here */
    return HCELL_NIL;
}

BOOLEAN
NTAPI
CmpAddSubKey(IN PHHIVE Hive,
             IN HCELL_INDEX Parent,
             IN HCELL_INDEX Child)
{
    PCM_KEY_NODE KeyNode;
    PCM_KEY_INDEX Index;
    PCM_KEY_FAST_INDEX OldIndex;
    UNICODE_STRING Name;
    HCELL_INDEX IndexCell = HCELL_NIL, CellToRelease = HCELL_NIL, LeafCell;
    PHCELL_INDEX RootPointer = NULL;
    ULONG Type, i;
    BOOLEAN IsCompressed;
    PAGED_CODE();

    /* Get the key node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Child);
    if (!KeyNode)
    {
        /* Shouldn't happen */
        ASSERT(FALSE);
        return FALSE;
    }

    /* Check if the name is compressed */
    if (KeyNode->Flags & KEY_COMP_NAME)
    {
        /* Remember for later */
        IsCompressed = TRUE;

        /* Create the compressed name and allocate it */
        Name.Length = CmpCompressedNameSize(KeyNode->Name, KeyNode->NameLength);
        Name.MaximumLength = Name.Length;
        Name.Buffer = Hive->Allocate(Name.Length, TRUE, TAG_CM);
        if (!Name.Buffer)
        {
            /* Release the cell and fail */
            HvReleaseCell(Hive, Child);
            ASSERT(FALSE);
            return FALSE;
        }

        /* Copy the compressed name */
        CmpCopyCompressedName(Name.Buffer,
                              Name.MaximumLength,
                              KeyNode->Name,
                              KeyNode->NameLength);
    }
    else
    {
        /* Remember for later */
        IsCompressed = FALSE;

        /* Build the unicode string */
        Name.Length = KeyNode->NameLength;
        Name.MaximumLength = KeyNode->NameLength;
        Name.Buffer = &KeyNode->Name[0];
    }

    /* Release the cell */
    HvReleaseCell(Hive, Child);

    /* Get the parent node */
    KeyNode = (PCM_KEY_NODE)HvGetCell(Hive, Parent);
    if (!KeyNode)
    {
        /* Not handled */
        ASSERT(FALSE);
    }

    /* Find out the type of the cell, and check if this is the first subkey */
    Type = HvGetCellType(Child);
    if (!KeyNode->SubKeyCounts[Type])
    {
        /* Allocate a fast leaf */
        IndexCell = HvAllocateCell(Hive, sizeof(CM_KEY_FAST_INDEX), Type, HCELL_NIL);
        if (IndexCell == HCELL_NIL)
        {
            /* Not handled */
            ASSERT(FALSE);
        }

        /* Get the leaf cell */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
        if (!Index)
        {
            /* Shouldn't happen */
            ASSERT(FALSE);
        }

        /* Now check what kind of hive we're dealing with */
        if (Hive->Version >= 5)
        {
            /* XP Hive: Use hash leaf */
            Index->Signature = CM_KEY_HASH_LEAF;
        }
        else if (Hive->Version >= 3)
        {
            /* Windows 2000 and ReactOS: Use fast leaf */
            Index->Signature = CM_KEY_FAST_LEAF;
        }
        else
        {
            /* NT 4: Use index leaf */
            Index->Signature = CM_KEY_INDEX_LEAF;
        }

        /* Setup the index list */
        Index->Count = 0;
        KeyNode->SubKeyLists[Type] = IndexCell;
    }
    else
    {
        /* We already have an index, get it */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, KeyNode->SubKeyLists[Type]);
        if (!Index)
        {
            /* Not handled */
            ASSERT(FALSE);
        }

        /* Remember to release the cell later */
        CellToRelease = KeyNode->SubKeyLists[Type];

        /* Check if this is a fast leaf that's gotten too full */
        if ((Index->Signature == CM_KEY_FAST_LEAF) &&
            (Index->Count >= CmpMaxFastIndexPerHblock))
        {
            DPRINT("Doing Fast->Slow Leaf conversion\n");

            /* Mark this cell as dirty */
            HvMarkCellDirty(Hive, CellToRelease, FALSE);

            /* Convert */
            OldIndex = (PCM_KEY_FAST_INDEX)Index;

            for (i = 0; i < OldIndex->Count; i++)
            {
                Index->List[i] = OldIndex->List[i].Cell;
            }

            /* Set the new type value */
            Index->Signature = CM_KEY_INDEX_LEAF;
        }
        else if (((Index->Signature == CM_KEY_INDEX_LEAF) ||
                  (Index->Signature == CM_KEY_HASH_LEAF)) &&
                  (Index->Count >= CmpMaxIndexPerHblock))
        {
            /* This is an old/hashed leaf that's gotten too large, root it */
            IndexCell = HvAllocateCell(Hive,
                                      sizeof(CM_KEY_INDEX) +
                                      sizeof(HCELL_INDEX),
                                      Type,
                                      HCELL_NIL);
            if (IndexCell == HCELL_NIL)
            {
                /* Not handled */
                ASSERT(FALSE);
            }

            /* Get the index cell */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, IndexCell);
            if (!Index)
            {
                /* Shouldn't happen */
                ASSERT(FALSE);
            }

            /* Mark the index as a root, and set the index cell */
            Index->Signature = CM_KEY_INDEX_ROOT;
            Index->Count = 1;
            Index->List[0] = KeyNode->SubKeyLists[Type];
            KeyNode->SubKeyLists[Type] = IndexCell;
        }
    }

    /* Now we can choose the leaf cell */
    LeafCell = KeyNode->SubKeyLists[Type];

    /* Check if we turned the index into a root */
    if (Index->Signature == CM_KEY_INDEX_ROOT)
    {
        DPRINT("Leaf->Root Index Conversion\n");

        /* Get the leaf where to add the new entry (the routine will do
         * the splitting if necessary)
         */
        LeafCell = CmpSelectLeaf(Hive, KeyNode, &Name, Type, &RootPointer);
        if (LeafCell == HCELL_NIL)
        {
            /* Not handled */
            ASSERT(FALSE);
        }
    }

    /* Add our leaf cell */
    LeafCell = CmpAddToLeaf(Hive, LeafCell, Child, &Name);
    if (LeafCell == HCELL_NIL)
    {
        /* Not handled */
        ASSERT(FALSE);
    }

    /* Update the key counts */
    KeyNode->SubKeyCounts[Type]++;

    /* Check if caller wants us to return the leaf */
    if (RootPointer)
    {
        /* Return it */
        *RootPointer = LeafCell;
    }
    else
    {
        /* Otherwise, mark it as the list index for the cell */
        KeyNode->SubKeyLists[Type] = LeafCell;
    }

    /* If the name was compressed, free our copy */
    if (IsCompressed) Hive->Free(Name.Buffer, 0);

    /* Release all our cells */
    if (IndexCell != HCELL_NIL) HvReleaseCell(Hive, IndexCell);
    if (CellToRelease != HCELL_NIL) HvReleaseCell(Hive, CellToRelease);
    HvReleaseCell(Hive, Parent);
    return TRUE;
}

BOOLEAN
NTAPI
CmpRemoveSubKey(IN PHHIVE Hive,
                IN HCELL_INDEX ParentKey,
                IN HCELL_INDEX TargetKey)
{
    PCM_KEY_NODE Node;
    UNICODE_STRING SearchName;
    BOOLEAN IsCompressed;
    WCHAR Buffer[50];
    HCELL_INDEX RootCell = HCELL_NIL, LeafCell, ChildCell;
    PCM_KEY_INDEX Root = NULL, Leaf;
    PCM_KEY_FAST_INDEX Child;
    ULONG Storage, RootIndex = INVALID_INDEX, LeafIndex;
    BOOLEAN Result = FALSE;
    HCELL_INDEX CellToRelease1 = HCELL_NIL, CellToRelease2  = HCELL_NIL;

    /* Get the target key node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, TargetKey);
    if (!Node) return FALSE;

    /* Make sure it's dirty, then release it */
    ASSERT(HvIsCellDirty(Hive, TargetKey));
    HvReleaseCell(Hive, TargetKey);

    /* Check if the name is compressed */
    if (Node->Flags & KEY_COMP_NAME)
    {
        /* Remember for later */
        IsCompressed = TRUE;

        /* Build the search name */
        SearchName.Length = CmpCompressedNameSize(Node->Name,
                                                  Node->NameLength);
        SearchName.MaximumLength = SearchName.Length;

        /* Do we need an extra bufer? */
        if (SearchName.MaximumLength > sizeof(Buffer))
        {
            /* Allocate one */
            SearchName.Buffer = CmpAllocate(SearchName.Length,
                                            TRUE,
                                            TAG_CM);
            if (!SearchName.Buffer) return FALSE;
        }
        else
        {
            /* Otherwise, use our local stack buffer */
            SearchName.Buffer = Buffer;
        }

        /* Copy the compressed name */
        CmpCopyCompressedName(SearchName.Buffer,
                              SearchName.MaximumLength,
                              Node->Name,
                              Node->NameLength);
    }
    else
    {
        /* It's not compressed, build the name directly from the node */
        IsCompressed = FALSE;
        SearchName.Length = Node->NameLength;
        SearchName.MaximumLength = Node->NameLength;
        SearchName.Buffer = Node->Name;
    }

    /* Now get the parent node */
    Node = (PCM_KEY_NODE)HvGetCell(Hive, ParentKey);
    if (!Node) goto Exit;

    /* Make sure it's dirty, then release it */
    ASSERT(HvIsCellDirty(Hive, ParentKey));
    HvReleaseCell(Hive, ParentKey);

    /* Get the storage type and make sure it's not empty */
    Storage = HvGetCellType(TargetKey);
    ASSERT(Node->SubKeyCounts[Storage] != 0);
    //ASSERT(HvIsCellAllocated(Hive, Node->SubKeyLists[Storage]));

    /* Get the leaf cell now */
    LeafCell = Node->SubKeyLists[Storage];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Exit;

    /* Remember to release it later */
    CellToRelease1 = LeafCell;

    /* Check if the leaf is a root */
    if (Leaf->Signature == CM_KEY_INDEX_ROOT)
    {
        /* Find the child inside the root */
        RootIndex = CmpFindSubKeyInRoot(Hive, Leaf, &SearchName, &ChildCell);
        if (RootIndex & INVALID_INDEX) goto Exit;
        ASSERT(ChildCell != FALSE);

        /* The root cell is now this leaf */
        Root = Leaf;
        RootCell = LeafCell;

        /* And the new leaf is now this child */
        LeafCell = ChildCell;
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if (!Leaf) goto Exit;

        /* Remember to release it later */
        CellToRelease2 = LeafCell;
    }

    /* Make sure the leaf is valid */
    ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
           (Leaf->Signature == CM_KEY_FAST_LEAF) ||
           (Leaf->Signature == CM_KEY_HASH_LEAF));

    /* Now get the child in the leaf */
    LeafIndex = CmpFindSubKeyInLeaf(Hive, Leaf, &SearchName, &ChildCell);
    if (LeafIndex & INVALID_INDEX) goto Exit;
    ASSERT(ChildCell != HCELL_NIL);

    /* Decrement key counts and check if this was the last leaf entry */
    Node->SubKeyCounts[Storage]--;
    if (!(--Leaf->Count))
    {
        /* Free the leaf */
        HvFreeCell(Hive, LeafCell);

        /* Check if we were inside a root */
        if (Root)
        {
            /* Decrease the root count too, since the leaf is going away */
            if (!(--Root->Count))
            {
                /* The root is gone too,n ow */
                HvFreeCell(Hive, RootCell);
                Node->SubKeyLists[Storage] = HCELL_NIL;
            }
            else if (RootIndex < Root->Count)
            {
                /* Bring everything up by one */
                RtlMoveMemory(&Root->List[RootIndex],
                              &Root->List[RootIndex + 1],
                              (Root->Count - RootIndex) * sizeof(HCELL_INDEX));
            }
        }
        else
        {
            /* Otherwise, just clear the cell */
            Node->SubKeyLists[Storage] = HCELL_NIL;
        }
    }
    else if (LeafIndex < Leaf->Count)
    {
        /* Was the leaf a normal index? */
        if (Leaf->Signature == CM_KEY_INDEX_LEAF)
        {
            /* Bring everything up by one */
            RtlMoveMemory(&Leaf->List[LeafIndex],
                          &Leaf->List[LeafIndex + 1],
                          (Leaf->Count - LeafIndex) * sizeof(HCELL_INDEX));
        }
        else
        {
            /* This is a fast index, bring everything up by one */
            Child = (PCM_KEY_FAST_INDEX)Leaf;
            RtlMoveMemory(&Child->List[LeafIndex],
                          &Child->List[LeafIndex+1],
                          (Child->Count - LeafIndex) * sizeof(CM_INDEX));
        }
    }

    /* If we got here, now we're done */
    Result = TRUE;

Exit:
    /* Release any cells we may have been holding */
    if (CellToRelease1 != HCELL_NIL) HvReleaseCell(Hive, CellToRelease1);
    if (CellToRelease2 != HCELL_NIL) HvReleaseCell(Hive, CellToRelease2);

    /* Check if the name was compressed and not inside our local buffer */
    if ((IsCompressed) && (SearchName.MaximumLength > sizeof(Buffer)))
    {
        /* Free the buffer we allocated */
        CmpFree(SearchName.Buffer, 0);
    }

    /* Return the result */
    return Result;
}
