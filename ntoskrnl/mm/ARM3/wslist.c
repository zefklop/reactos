/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/wslist.c
 * PURPOSE:         ARM Memory Manager Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "miarm.h"

#define MMWSLE_PER_PAGE (PAGE_SIZE / sizeof(MMWSLE))

/* GLOBALS ********************************************************************/
PMMWSL MmWorkingSetList;
KEVENT MmWorkingSetManagerEvent;

/* PRIVATE FUNCTIONS **********************************************************/
static
ULONG
MiGetFirstFreeWsleIndex(_Inout_ PMMSUPPORT Vm)
{
    PMMWSL WsList = Vm->VmWorkingSetList;

    /* Some sanity checks */
    ASSERT((WsList->FirstFree < WsList->LastEntry) || (WsList->FirstFree == MMWSLE_NEXT_FREE_INVALID));

    /* Check if we are initializing */
    if (WsList->LastEntry == MAXULONG)
    {
        ASSERT(WsList->FirstDynamic < WsList->LastInitializedWsle);
        return WsList->FirstDynamic++;
    }

    ASSERT(WsList->LastEntry <= WsList->LastInitializedWsle);
    /* See if our first free entry is valid */
    if (WsList->FirstFree == MMWSLE_NEXT_FREE_INVALID)
    {
        if (WsList->LastEntry == WsList->LastInitializedWsle)
        {
            /* We must grow our array. Allocate a new page */
            PVOID Address = &WsList->Wsle[WsList->LastInitializedWsle + 1];
            PMMPTE PointerPte = MiAddressToPte(Address);
            MMPTE TempPte;

            MI_SET_USAGE(MI_USAGE_WSLE);
            MI_SET_WORKING_SET(Vm);

            /* We must be at page boundary */
            ASSERT(Address == ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE));

            PFN_NUMBER PageFrameIndex = MiRemoveAnyPage(MI_GET_NEXT_COLOR());

            MiInitializePfn(PageFrameIndex, PointerPte, TRUE);

            TempPte = Vm == &MmSystemCacheWs ? ValidKernelPte : ValidKernelPteLocal;
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE(PointerPte, TempPte);

            WsList->LastInitializedWsle += MMWSLE_PER_PAGE;

            /* Make sure we are staying on the same page */
            ASSERT(Address == ALIGN_DOWN_POINTER_BY(&WsList->Wsle[WsList->LastInitializedWsle], PAGE_SIZE));
        }

        /* At this point we must be good to go */
        ASSERT(WsList->LastEntry < WsList->LastInitializedWsle);

        return ++WsList->LastEntry;
    }
    else
    {
        ULONG WsIndex = WsList->FirstFree;
        PMMWSLE WsleEntry = &WsList->Wsle[WsIndex];

        ASSERT(WsIndex < WsList->LastEntry);

        ASSERT(WsleEntry->u1.Free.MustBeZero == 0);
        ASSERT(WsleEntry->u1.Free.PreviousFree == MMWSLE_PREVIOUS_FREE_INVALID);
        ASSERT(WsleEntry->u1.Free.NextFree > WsIndex);

        if (WsleEntry->u1.Free.NextFree != MMWSLE_NEXT_FREE_INVALID)
        {
            PMMWSLE NextFree = &WsList->Wsle[WsleEntry->u1.Free.NextFree];

            ASSERT(NextFree->u1.Free.MustBeZero == 0);
            ASSERT(NextFree->u1.Free.PreviousFree == (NextFree - WsleEntry) % MMWSLE_PER_PAGE);
            NextFree->u1.Free.PreviousFree = MMWSLE_PREVIOUS_FREE_INVALID;
        }

        WsList->FirstFree = WsleEntry->u1.Free.NextFree;

        return WsIndex;
    }
}

VOID
NTAPI
MiInsertInWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address,
    _In_ ULONG Protection)
{
    ULONG WsIndex = MiGetFirstFreeWsleIndex(Vm);
    PMMWSLE WsleEntry = &Vm->VmWorkingSetList->Wsle[WsIndex];
    PMMPTE PointerPte = MiAddressToPte(Address);
    PMMPFN Pfn1;

    /* Make sure we got a rounded address */
    Address = ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE);

    /* Make sure we are locking the right things */
    ASSERT(MM_ANY_WS_LOCK_HELD(PsGetCurrentThread()));
    MI_ASSERT_PFN_LOCK_HELD();

    /* Make sure we are adding a paged-in address */
    ASSERT(PointerPte->u.Hard.Valid == 1);
    Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));

    /* The Pfn must be an active one */
    ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);

    WsleEntry->u1.Long = 0;
    WsleEntry->u1.VirtualAddress = Address;
    WsleEntry->u1.e1.Protection = Protection;

    /* Shared pages not supported yet */
    ASSERT(Pfn1->u1.WsIndex == 0);
    ASSERT(Pfn1->u3.e1.PrototypePte == 0);

    /* Nor are "ROS PFN" */
    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);

    WsleEntry->u1.e1.Direct = 1;

    Pfn1->u1.WsIndex = WsIndex;
    WsleEntry->u1.e1.Valid = 1;

    Vm->WorkingSetSize += PAGE_SIZE;
    if (Vm->WorkingSetSize > Vm->PeakWorkingSetSize)
        Vm->PeakWorkingSetSize = Vm->WorkingSetSize;
}

static
void
MiShrinkWorkingSet(_Inout_ PMMSUPPORT Vm)
{
    PMMWSL WsList = Vm->VmWorkingSetList;
    ULONG LastValid = WsList->LastEntry;

    while(WsList->Wsle[LastValid].u1.e1.Valid == 0)
    {
        ASSERT(LastValid != 0);
        LastValid--;
    }

    if (LastValid != WsList->LastEntry)
    {
        /* There was a hole behind us. Handle this */
        PMMWSLE Entry = &WsList->Wsle[LastValid + 1];

        if (WsList->FirstFree == LastValid + 1)
        {
            /* This was actually our first free entry. */
            WsList->FirstFree = MMWSLE_NEXT_FREE_INVALID;
        }
        else
        {
            PMMWSLE PreviousFree = Entry - Entry->u1.Free.PreviousFree;
            while (PreviousFree->u1.e1.Valid)
                PreviousFree -= MMWSLE_PER_PAGE;

            ASSERT(PreviousFree->u1.Free.NextFree == LastValid + 1);
            PreviousFree->u1.Free.NextFree = MMWSLE_NEXT_FREE_INVALID;
        }

        /* Nuke everyone */
        RtlZeroMemory(Entry, (WsList->LastEntry - LastValid) * sizeof(MMWSLE));
        WsList->LastEntry = LastValid;
    }

    if (LastValid < WsList->FirstDynamic)
    {
        /* Do not mess around with the protected ones */
        return;
    }

    /* See if we should shrink our array. */
    while ((WsList->LastInitializedWsle > MMWSLE_PER_PAGE)
        && (LastValid <= (WsList->LastInitializedWsle - MMWSLE_PER_PAGE)))
    {
        PVOID WsleArrayQueue = &WsList->Wsle[WsList->LastInitializedWsle];
        PMMPTE PointerPte = MiAddressToPte(WsleArrayQueue);
        PFN_NUMBER Page = PFN_FROM_PTE(PointerPte);
        PMMPFN Pfn = MiGetPfnEntry(Page);

        MI_SET_PFN_DELETED(Pfn);
        MiDecrementShareCount(MiGetPfnEntry(Pfn->u4.PteFrame), Pfn->u4.PteFrame);
        MiDecrementShareCount(Pfn, Page);

        PointerPte->u.Long = 0;

        KeInvalidateTlbEntry(WsleArrayQueue);

        WsList->LastInitializedWsle -= MMWSLE_PER_PAGE;
    }
}

VOID
NTAPI
MiRemoveFromWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address)
{
    PMMWSL WsList = Vm->VmWorkingSetList;
    ULONG WsIndex;
    PMMWSLE WsleEntry;
    PMMPTE PointerPte = MiAddressToPte(Address);
    PMMPFN Pfn1;

    /* Make sure we got a rounded address */
    Address = ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE);

    /* Make sure we are locking the right things */
    ASSERT(MM_ANY_WS_LOCK_HELD(PsGetCurrentThread()));
    MI_ASSERT_PFN_LOCK_HELD();

    /* Make sure we are removing a paged-in address */
    ASSERT(PointerPte->u.Hard.Valid == 1);
    Pfn1 = MiGetPfnEntry(PFN_FROM_PTE(PointerPte));

    /* The Pfn must be an active one */
    ASSERT(Pfn1->u3.e1.PageLocation == ActiveAndValid);

    WsIndex = Pfn1->u1.WsIndex;
    WsleEntry = &Vm->VmWorkingSetList->Wsle[WsIndex];

    /* Shared page not handled yet */
    ASSERT(Pfn1->u3.e1.PrototypePte == 0);
    ASSERT(WsleEntry->u1.e1.Direct == 1);
    /* Nor are "ROS PFN" */
    ASSERT(MI_IS_ROS_PFN(Pfn1) == FALSE);

    /* Some sanity checks */
    ASSERT(WsIndex >= WsList->FirstDynamic);
    ASSERT(WsIndex <= WsList->LastEntry);
    ASSERT(WsIndex <= WsList->LastInitializedWsle);
    ASSERT(WsleEntry->u1.e1.Valid == 1);
    ASSERT(WsleEntry->u1.e1.VirtualPageNumber == ((ULONG_PTR)Address) >> PAGE_SHIFT);

    /* Let this go */
    Pfn1->u1.WsIndex = 0;

    /* Nuke it */
    WsleEntry->u1.Long = 0;

    /* Insert our entry into the free list */
    if (WsIndex == WsList->LastEntry)
    {
        ASSERT(WsIndex != 0);
        /* Let's shrink the active list */
        WsList->LastEntry--;
        MiShrinkWorkingSet(Vm);
    }
    else if (WsList->FirstFree == MMWSLE_NEXT_FREE_INVALID)
    {
        /* We are the first free entry to be inserted */
        WsList->FirstFree = WsIndex;
        WsleEntry->u1.Free.PreviousFree = MMWSLE_PREVIOUS_FREE_INVALID;
        WsleEntry->u1.Free.NextFree = MMWSLE_NEXT_FREE_INVALID;
    }
    else
    {
        /* Keep this sorted */
        PMMWSLE NextFree = &WsList->Wsle[WsList->FirstFree];
        PMMWSLE PreviousFree = NULL;

        ASSERT(NextFree->u1.Free.MustBeZero == 0);

        while (NextFree < WsleEntry)
        {
            PreviousFree = NextFree;
            if (NextFree->u1.Free.NextFree != MMWSLE_NEXT_FREE_INVALID)
            {
                NextFree = &WsList->Wsle[NextFree->u1.Free.NextFree];
                ASSERT(NextFree->u1.Free.MustBeZero == 0);
            }
            else
            {
                NextFree = NULL;
                break;
            }
        }

        ASSERT(PreviousFree || NextFree);

        if (PreviousFree)
        {
        	ULONG PreviousFreeIndex = PreviousFree - WsList->Wsle;
            ASSERT((NextFree != NULL) || (PreviousFree->u1.Free.NextFree == MMWSLE_NEXT_FREE_INVALID));
            PreviousFree->u1.Free.NextFree = WsIndex;
            /* The PreviousFree entry is a relative index into the current page */
            WsleEntry->u1.Free.PreviousFree = (WsIndex - PreviousFreeIndex) % MMWSLE_PER_PAGE;
            ASSERT(WsleEntry->u1.Free.PreviousFree < MMWSLE_PREVIOUS_FREE_INVALID);
        }
        else
        {
            WsleEntry->u1.Free.PreviousFree = MMWSLE_PREVIOUS_FREE_INVALID;
            ASSERT(NextFree->u1.Free.PreviousFree == MMWSLE_PREVIOUS_FREE_INVALID);
            WsList->FirstFree = WsIndex;
        }

        if (NextFree)
        {
            ULONG NextFreeIndex = NextFree - WsList->Wsle;

            /* The PreviousFree entry is a relative index into the current page */
            NextFree->u1.Free.PreviousFree = (NextFreeIndex - WsIndex) % MMWSLE_PER_PAGE;

            WsleEntry->u1.Free.NextFree = (NextFree - WsList->Wsle);
        }
        else
        {
            WsleEntry->u1.Free.NextFree = MMWSLE_NEXT_FREE_INVALID;
        }
    }

    Vm->WorkingSetSize -= PAGE_SIZE;
}

static
VOID
MiTrimWorkingSet(PMMSUPPORT Vm)
{
    ULONG i;
    PMMWSL WsList;

    /* This should be done under WS lock */
    ASSERT(MM_ANY_WS_LOCK_HELD(PsGetCurrentThread()));

    /* Mark this as being trimmed */
    Vm->Flags.BeingTrimmed = 1;

    /* Walk the WsList */
    WsList = Vm->VmWorkingSetList;
    for (i = WsList->FirstDynamic; i < WsList->LastEntry; i++)
    {
        PMMWSLE Entry = &WsList->Wsle[i];
        PMMPTE PointerPte;
        MMPTE TempPte;
        KIRQL OldIrql;
        PMMPFN Pfn1;
        PFN_NUMBER Page;
        ULONG Protection;

        if (!Entry->u1.e1.Valid)
            continue;

        /* Only direct entries for now */
        ASSERT(Entry->u1.e1.Direct == 1);

        /* Check the PTE */
        PointerPte = MiAddressToPte(Entry->u1.VirtualAddress);

        /* This must be valid */
        ASSERT(PointerPte->u.Hard.Valid);

        /* If the PTE was accessed, simply reset and that's the end of it */
        if (PointerPte->u.Hard.Accessed)
        {
            Entry->u1.e1.Age = 0;
            PointerPte->u.Hard.Accessed = 0;
            KeInvalidateTlbEntry(Entry->u1.VirtualAddress);
            continue;
        }

        /* If the entry is not so old, just age it
         * We make it older if we must trim hard. */
        if ((Entry->u1.e1.Age + Vm->Flags.TrimHard) < 3)
        {
            Entry->u1.e1.Age++;
            continue;
        }

        if ((Entry->u1.e1.LockedInMemory) || (Entry->u1.e1.LockedInWs))
        {
            /* This one is locked. Next time, maybe... */
            continue;
        }

        /* FIXME: Invalidating PDEs breaks legacy MMs */
        if (MI_IS_PAGE_TABLE_ADDRESS(Entry->u1.VirtualAddress))
            continue;

        /* Please put yourself aside and make place for the younger ones */
        Page = PFN_FROM_PTE(PointerPte);
        OldIrql = MiAcquirePfnLock();

        Pfn1 = MiGetPfnEntry(Page);

        /* Not supported yet */
        ASSERT(Pfn1->u3.e1.PrototypePte == 0);
        ASSERT(!MI_IS_ROS_PFN(Pfn1));

        /* FIXME: Remove this hack when possible */
        if (Pfn1->Wsle.u1.e1.LockedInMemory || (Pfn1->Wsle.u1.e1.LockedInWs))
        {
            MiReleasePfnLock(OldIrql);
            continue;
        }

        /* We can remove it from the list. Save Protection first */
        Protection = Entry->u1.e1.Protection;
        MiRemoveFromWorkingSetList(Vm, Entry->u1.VirtualAddress);

        /* Dirtify the page, if needed */
        if (PointerPte->u.Hard.Dirty)
            Pfn1->u3.e1.Modified = 1;

        /* Make this a transition PTE */
        MI_MAKE_TRANSITION_PTE(&TempPte, Page, Protection);
        MI_WRITE_INVALID_PTE(PointerPte, TempPte);
        KeInvalidateTlbEntry(MiAddressToPte(PointerPte));

        /* Drop the share count. This will take care of putting it in the standby or modified list. */
        MiDecrementShareCount(Pfn1, Page);

        /* Are we done yet ? Let's see */
        if ((MmAvailablePages > MmMinimumFreePages) &&
            (MmAvailablePages + MmModifiedPageListHead.Total) > MmPlentyFreePages)
        {
            /* Yes ! */
            MiReleasePfnLock(OldIrql);
            break;
        }

        MiReleasePfnLock(OldIrql);
    }

    /* We're done */
    Vm->Flags.BeingTrimmed = 0;
}

VOID
NTAPI
MmWorkingSetManager(VOID)
{
    PLIST_ENTRY VmListEntry;
    PMMSUPPORT Vm = NULL;
    KIRQL OldIrql;

    OldIrql = MiAcquireExpansionLock();

    for (VmListEntry = MmWorkingSetExpansionHead.Flink;
         VmListEntry != &MmWorkingSetExpansionHead;
         VmListEntry = VmListEntry->Flink)
    {
        BOOLEAN TrimHard = MmAvailablePages < MmMinimumFreePages;
        PEPROCESS Process = NULL;

        /* Don't do anything if we have plenty of free pages. */
        if ((MmAvailablePages + MmModifiedPageListHead.Total) >= MmPlentyFreePages)
            break;

        Vm = CONTAINING_RECORD(VmListEntry, MMSUPPORT, WorkingSetExpansionLinks);

        /* Let the legacy Mm System space alone */
        if (Vm == MmGetKernelAddressSpace())
            continue;

        if (MI_IS_PROCESS_WORKING_SET(Vm))
        {
            Process = CONTAINING_RECORD(Vm, EPROCESS, Vm);

            /* Make sure the process is not terminating abd attach to it */
            if (!ExAcquireRundownProtection(&Process->RundownProtect))
                continue;
            ASSERT(!KeIsAttachedProcess());
            KeAttachProcess(&Process->Pcb);
        }
        else
        {
            /* FIXME: Session & system space unsupported */
            continue;
        }

        MiReleaseExpansionLock(OldIrql);

        /* Share-lock for now, we're only reading */
        MiLockWorkingSetShared(PsGetCurrentThread(), Vm);

        if (((Vm->WorkingSetSize > Vm->MaximumWorkingSetSize) ||
            (TrimHard && (Vm->WorkingSetSize > Vm->MinimumWorkingSetSize))) &&
            MiConvertSharedWorkingSetLockToExclusive(PsGetCurrentThread(), Vm))
        {
            /* Tell the trimmer we're in dire need */
            if (TrimHard)
                Vm->Flags.TrimHard = 1;

            MiTrimWorkingSet(Vm);

            /* Reset this flag */
            Vm->Flags.TrimHard = 0;

            MiUnlockWorkingSet(PsGetCurrentThread(), Vm);
        }
        else
        {
            MiUnlockWorkingSetShared(PsGetCurrentThread(), Vm);
        }

        /* Lock again */
        OldIrql = MiAcquireExpansionLock();

        if (Process)
        {
            KeDetachProcess();
            ExReleaseRundownProtection(&Process->RundownProtect);
        }
    }

    MiReleaseExpansionLock(OldIrql);

    /* Now make sure that the pages get to the page file and that we didn't trim for nothing */
    MiModifiedPageWrite(FALSE);
}
