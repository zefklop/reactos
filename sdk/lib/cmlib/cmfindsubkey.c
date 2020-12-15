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

HCELL_INDEX
NTAPI
CmpDoFindSubKeyByNumber(IN PHHIVE Hive,
                        IN PCM_KEY_INDEX Index,
                        IN ULONG Number)
{
    ULONG i;
    HCELL_INDEX LeafCell = 0;
    PCM_KEY_INDEX Leaf = NULL;
    PCM_KEY_FAST_INDEX FastIndex;
    HCELL_INDEX Result;

    /* Check if this is a root */
    if (Index->Signature == CM_KEY_INDEX_ROOT)
    {
        /* Loop the index */
        for (i = 0; i < Index->Count; i++)
        {
            /* Check if this isn't the first iteration */
            if (i)
            {
                /* Make sure we had something valid, and release it */
                ASSERT(Leaf != NULL );
                ASSERT(LeafCell == Index->List[i - 1]);
                HvReleaseCell(Hive, LeafCell);
            }

            /* Get the leaf cell and the leaf for this index */
            LeafCell = Index->List[i];
            Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
            if (!Leaf) return HCELL_NIL;

            /* Check if the index may be inside it */
            if (Number < Leaf->Count)
            {
                /* Check if this is a fast or hash leaf */
                if ((Leaf->Signature == CM_KEY_FAST_LEAF) ||
                    (Leaf->Signature == CM_KEY_HASH_LEAF))
                {
                    /* Get the fast index */
                    FastIndex = (PCM_KEY_FAST_INDEX)Leaf;

                    /* Look inside the list to get our actual cell */
                    Result = FastIndex->List[Number].Cell;
                    HvReleaseCell(Hive, LeafCell);
                    return Result;
                }
                else
                {
                    /* Regular leaf, so just use the index directly */
                    Result = Leaf->List[Number];

                    /*  Release and return it */
                    HvReleaseCell(Hive,LeafCell);
                    return Result;
                }
            }
            else
            {
                /* Otherwise, skip this leaf */
                Number = Number - Leaf->Count;
            }
        }

        /* Should never get here */
        ASSERT(FALSE);
    }

    /* If we got here, then the cell is in this index */
    ASSERT(Number < Index->Count);

    /* Check if we're a fast or hash leaf */
    if ((Index->Signature == CM_KEY_FAST_LEAF) ||
        (Index->Signature == CM_KEY_HASH_LEAF))
    {
        /* We are, get the fast index and get the cell from the list */
        FastIndex = (PCM_KEY_FAST_INDEX)Index;
        return FastIndex->List[Number].Cell;
    }
    else
    {
        /* We aren't, so use the index directly to grab the cell */
        return Index->List[Number];
    }
}

HCELL_INDEX
NTAPI
CmpFindSubKeyByNumber(IN PHHIVE Hive,
                      IN PCM_KEY_NODE Node,
                      IN ULONG Number)
{
    PCM_KEY_INDEX Index;
    HCELL_INDEX Result = HCELL_NIL;

    /* Check if it's in the stable list */
    if (Number < Node->SubKeyCounts[Stable])
    {
        /* Get the actual key index */
        Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Stable]);
        if (!Index) return HCELL_NIL;

        /* Do a search inside it */
        Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);

        /* Release the cell and return the result */
        HvReleaseCell(Hive, Node->SubKeyLists[Stable]);
        return Result;
    }
    else if (Hive->StorageTypeCount > Volatile)
    {
        /* It's in the volatile list */
        Number = Number - Node->SubKeyCounts[Stable];
        if (Number < Node->SubKeyCounts[Volatile])
        {
            /* Get the actual key index */
            Index = (PCM_KEY_INDEX)HvGetCell(Hive, Node->SubKeyLists[Volatile]);
            if (!Index) return HCELL_NIL;

            /* Do a search inside it */
            Result = CmpDoFindSubKeyByNumber(Hive, Index, Number);

            /* Release the cell and return the result */
            HvReleaseCell(Hive, Node->SubKeyLists[Volatile]);
            return Result;
        }
    }

    /* Nothing was found */
    return HCELL_NIL;
}

ULONG
NTAPI
CmpFindSubKeyInRoot(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey)
{
    ULONG High, Low = 0, i = 0, ReturnIndex;
    HCELL_INDEX LeafCell;
    PCM_KEY_INDEX Leaf;
    LONG Result;

    /* Verify Index for validity */
    ASSERT(Index->Count != 0);
    ASSERT(Index->Signature == CM_KEY_INDEX_ROOT);

    /* Set high limit and loop */
    High = Index->Count - 1;
    while (TRUE)
    {
        /* Choose next entry */
        i = ((High - Low) / 2) + Low;

        /* Get the leaf cell and then the leaf itself */
        LeafCell = Index->List[i];
        Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
        if (Leaf)
        {
            /* Make sure the leaf is valid */
            ASSERT((Leaf->Signature == CM_KEY_INDEX_LEAF) ||
                   (Leaf->Signature == CM_KEY_FAST_LEAF) ||
                   (Leaf->Signature == CM_KEY_HASH_LEAF));
            ASSERT(Leaf->Count != 0);

            /* Do the compare */
            Result = CmpCompareInIndex(Hive,
                                       SearchName,
                                       Leaf->Count - 1,
                                       Leaf,
                                       SubKey);
            if (Result == 2) goto Big;

            /* Check if we found the leaf */
            if (!Result)
            {
                /* We found the leaf */
                *SubKey = LeafCell;
                ReturnIndex = i;
                goto Return;
            }

            /* Check for negative result */
            if (Result < 0)
            {
                /* If we got here, we should be at -1 */
                ASSERT(Result == -1);

                /* Do another lookup, since we might still be in the right leaf */
                Result = CmpCompareInIndex(Hive,
                                           SearchName,
                                           0,
                                           Leaf,
                                           SubKey);
                if (Result == 2) goto Big;

                /* Check if it's not below */
                if (Result >= 0)
                {
                    /*
                     * If the name was first below, and now it is above,
                     * then this means that it is somewhere in this leaf.
                     * Make sure we didn't get some weird result
                     */
                    ASSERT((Result == 1) || (Result == 0));

                    /* Return it */
                    *SubKey = LeafCell;
                    ReturnIndex = i;
                    goto Return;
                }

                /* Update the limit to this index, since we know it's not higher. */
                High = i;
            }
            else
            {
                /* Update the base to this index, since we know it's not lower. */
                Low = i;
            }
        }
        else
        {
Big:
            /* This was some sort of special key */
            ReturnIndex = INVALID_INDEX;
            goto ReturnFailure;
        }

        /* Check if there is only one entry left */
        if ((High - Low) <= 1) break;

        /* Release the leaf cell */
        HvReleaseCell(Hive, LeafCell);
    }

    /* Make sure we got here for the right reasons */
    ASSERT((High - Low == 1) || (High == Low));

    /* Get the leaf cell and the leaf */
    LeafCell = Index->List[Low];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Big;

    /* Do the compare */
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count-1,
                               Leaf,
                               SubKey);
    if (Result == 2) goto Big;

    /* Check if we found it */
    if (!Result)
    {
        /* We got lucky... return it */
        *SubKey = LeafCell;
        ReturnIndex = Low;
        goto Return;
    }

    /* It's below, so could still be in this leaf */
    if (Result < 0)
    {
        /* Make sure we're -1 */
        ASSERT(Result == -1);

        /* Do a search from the bottom */
        Result = CmpCompareInIndex(Hive, SearchName, 0, Leaf, SubKey);
        if (Result == 2) goto Big;

        /*
         * Check if it's above, which means that it's within the ranges of our
         * leaf (since we were below before).
         */
        if (Result >= 0)
        {
            /* Sanity check */
            ASSERT((Result == 1) || (Result == 0));

            /* Yep, so we're in the right leaf; return it. */
            *SubKey = LeafCell;
            ReturnIndex = Low;
            goto Return;
        }

        /* It's still below us, so fail */
        ReturnIndex = Low;
        goto ReturnFailure;
    }

    /* Release the leaf cell */
    HvReleaseCell(Hive, LeafCell);

    /* Well the low didn't work too well, so try the high. */
    LeafCell = Index->List[High];
    Leaf = (PCM_KEY_INDEX)HvGetCell(Hive, LeafCell);
    if (!Leaf) goto Big;

    /* Do the compare */
    Result = CmpCompareInIndex(Hive,
                               SearchName,
                               Leaf->Count - 1,
                               Leaf,
                               SubKey);
    if (Result == 2) goto Big;

    /* Check if we found it */
    if (Result == 0)
    {
        /* We got lucky... return it */
        *SubKey = LeafCell;
        ReturnIndex = High;
        goto Return;
    }

    /* Check if we are too high */
    if (Result < 0)
    {
        /* Make sure we're -1 */
        ASSERT(Result == -1);

        /*
         * Once again... since we were first too low and now too high, then
         * this means we are within the range of this leaf... return it.
         */
        *SubKey = LeafCell;
        ReturnIndex = High;
        goto Return;
    }

    /* If we got here, then we are too low, again. */
    ReturnIndex = High;

    /* Failure path */
ReturnFailure:
    *SubKey = HCELL_NIL;

    /* Return path...check if we have a leaf to free */
Return:
    if (Leaf) HvReleaseCell(Hive, LeafCell);

    /* Return the index */
    return ReturnIndex;
}

ULONG
NTAPI
CmpFindSubKeyInLeaf(IN PHHIVE Hive,
                    IN PCM_KEY_INDEX Index,
                    IN PCUNICODE_STRING SearchName,
                    IN PHCELL_INDEX SubKey)
{
    ULONG High, Low = 0, i;
    LONG Result;

    /* Verify Index for validity */
    ASSERT((Index->Signature == CM_KEY_INDEX_LEAF) ||
           (Index->Signature == CM_KEY_FAST_LEAF) ||
           (Index->Signature == CM_KEY_HASH_LEAF));

    /* Get the upper bound and middle entry */
    High = Index->Count - 1;
    i = High / 2;

    /* Check if we don't actually have any entries */
    if (!Index->Count)
    {
        /* Return failure */
        *SubKey = HCELL_NIL;
        return 0;
    }

    /* Start compare loop */
    while (TRUE)
    {
        /* Do the actual comparison and check the result */
        Result = CmpCompareInIndex(Hive, SearchName, i, Index, SubKey);
        if (Result == 2)
        {
            /* Fail with special value */
            *SubKey = HCELL_NIL;
            return INVALID_INDEX;
        }

        /* Check if we got lucky and found it */
        if (!Result) return i;

        /* Check if the result is below us */
        if (Result < 0)
        {
            /* Set the new bound; it can't be higher then where we are now. */
            ASSERT(Result == -1);
            High = i;
        }
        else
        {
            /* Set the new bound... it can't be lower then where we are now. */
            ASSERT(Result == 1);
            Low = i;
        }

        /* Check if this is the last entry, if so, break out and handle it */
        if ((High - Low) <= 1) break;

        /* Set the new index */
        i = ((High - Low) / 2) + Low;
    }

    /*
     * If we get here, High - Low = 1 or High == Low
     * Simply look first at Low, then at High
     */
    Result = CmpCompareInIndex(Hive, SearchName, Low, Index, SubKey);
    if (Result == 2)
    {
        /* Fail with special value */
        *SubKey = HCELL_NIL;
        return INVALID_INDEX;
    }

    /* Check if we got lucky and found it */
    if (!Result) return Low;

    /* Check if the result is below us */
    if (Result < 0)
    {
        /* Return the low entry */
        ASSERT(Result == -1);
        return Low;
    }

    /*
     * If we got here, then just check the high and return it no matter what
     * the result is (since we're a leaf, it has to be near there anyway).
     */
    Result = CmpCompareInIndex(Hive, SearchName, High, Index, SubKey);
    if (Result == 2)
    {
        /* Fail with special value */
        *SubKey = HCELL_NIL;
        return INVALID_INDEX;
    }

    /* Return the high */
    return High;
}

static HCELL_INDEX
NTAPI
CmpFindSubKeyByHash(IN PHHIVE Hive,
                    IN PCM_KEY_FAST_INDEX FastIndex,
                    IN PCUNICODE_STRING SearchName)
{
    ULONG HashKey, i;
    PCM_INDEX FastEntry;

    /* Make sure it's really a hash */
    ASSERT(FastIndex->Signature == CM_KEY_HASH_LEAF);

    /* Compute the hash key for the name */
    HashKey = CmpComputeHashKey(0, SearchName, FALSE);

    /* Loop all the entries */
    for (i = 0; i < FastIndex->Count; i++)
    {
        /* Get the entry */
        FastEntry = &FastIndex->List[i];

        /* Compare the hash first */
        if (FastEntry->HashKey == HashKey)
        {
            /* Go ahead for a full compare */
            if (!(CmpDoCompareKeyName(Hive, SearchName, FastEntry->Cell)))
            {
                /* It matched, return the cell */
                return FastEntry->Cell;
            }
        }
    }

    /* If we got here then we failed */
    return HCELL_NIL;
}

HCELL_INDEX
NTAPI
CmpFindSubKeyByName(IN PHHIVE Hive,
                    IN PCM_KEY_NODE Parent,
                    IN PCUNICODE_STRING SearchName)
{
    ULONG i;
    PCM_KEY_INDEX IndexRoot;
    HCELL_INDEX SubKey, CellToRelease;
    ULONG Found;

    /* Loop each storage type */
    for (i = 0; i < Hive->StorageTypeCount; i++)
    {
        /* Make sure the parent node has subkeys */
        if (Parent->SubKeyCounts[i])
        {
            /* Get the Index */
            IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, Parent->SubKeyLists[i]);
            if (!IndexRoot) return HCELL_NIL;

            /* Get the cell we'll need to release */
            CellToRelease = Parent->SubKeyLists[i];

            /* Check if this is another index root */
            if (IndexRoot->Signature == CM_KEY_INDEX_ROOT)
            {
                /* Lookup the name in the root */
                Found = CmpFindSubKeyInRoot(Hive,
                                            IndexRoot,
                                            SearchName,
                                            &SubKey);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);

                /* Make sure we found something valid */
                if (Found & INVALID_INDEX) break;

                /* Get the new Index Root and set the new cell to be released */
                if (SubKey == HCELL_NIL) continue;
                CellToRelease = SubKey;
                IndexRoot = (PCM_KEY_INDEX)HvGetCell(Hive, SubKey);
            }

            /* Make sure the signature is what we expect it to be */
            ASSERT((IndexRoot->Signature == CM_KEY_INDEX_LEAF) ||
                   (IndexRoot->Signature == CM_KEY_FAST_LEAF) ||
                   (IndexRoot->Signature == CM_KEY_HASH_LEAF));

            /* Check if this isn't a hashed leaf */
            if (IndexRoot->Signature != CM_KEY_HASH_LEAF)
            {
                /* Find the subkey in the leaf */
                Found = CmpFindSubKeyInLeaf(Hive,
                                            IndexRoot,
                                            SearchName,
                                            &SubKey);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);

                /* Make sure we found a valid index */
                if (Found & INVALID_INDEX) break;
            }
            else
            {
                /* Find the subkey in the hash */
                SubKey = CmpFindSubKeyByHash(Hive,
                                             (PCM_KEY_FAST_INDEX)IndexRoot,
                                             SearchName);

                /* Release the previous cell */
                ASSERT(CellToRelease != HCELL_NIL);
                HvReleaseCell(Hive, CellToRelease);
            }

            /* Make sure we got a valid subkey and return it */
            if (SubKey != HCELL_NIL) return SubKey;
        }
    }

    /* If we got here, then we failed */
    return HCELL_NIL;
}
