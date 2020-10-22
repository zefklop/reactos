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
    ASSERT(WsList->LastEntry <= WsList->LastInitializedWsle);

    /* Check if we are initializing */
    if (WsList->LastEntry == 0)
    {
        ASSERT(WsList->FirstDynamic < WsList->LastInitializedWsle);
        return WsList->FirstDynamic++;
    }

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
            MI_SET_PROCESS(PsGetCurrentProcess());

            /* We must be at page boundary */
            ASSERT(Address == ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE));

            PFN_NUMBER PageFrameIndex = MiRemoveAnyPage(MI_GET_NEXT_COLOR());

            MiInitializePfn(PageFrameIndex, PointerPte, TRUE);

            TempPte = ValidKernelPteLocal;
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE(PointerPte, TempPte);

            WsList->LastInitializedWsle += MMWSLE_PER_PAGE;

            /* Make sure we are staying on the same page */
            ASSERT(Address == ALIGN_DOWN_POINTER_BY(&WsList->Wsle[WsList->LastInitializedWsle], PAGE_SIZE));

            /* We must insert this page in our working set ! */
            MiInsertInWorkingSetList(Vm, Address, MM_READWRITE);

            /* Now the last entry is the tail of our WSLE array */
            ASSERT(WsList->Wsle[WsList->LastEntry].u1.e1.VirtualPageNumber == ((ULONG_PTR)Address) >> PAGE_SHIFT);
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

        if (WsleEntry->u1.Free.NextFree < WsList->LastEntry)
        {
            PMMWSLE_FREE_ENTRY NextFree = &WsList->Wsle[WsleEntry->u1.Free.NextFree].u1.Free;

            ASSERT(NextFree->MustBeZero == 0);
            NextFree->PreviousFree = MMWSLE_PREVIOUS_FREE_INVALID;

            WsList->FirstFree = WsleEntry->u1.Free.NextFree;
        }
        else
        {
        	WsList->FirstFree = MMWSLE_NEXT_FREE_INVALID;
        }

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
        LastValid--;
    }

    if (LastValid != WsList->LastEntry)
    {
        /* There was a hole behind us. Handle this */
        if (WsList->FirstFree == LastValid + 1)
        {
            /* This was actually our first free entry. */
            WsList->FirstFree = MMWSLE_NEXT_FREE_INVALID;
        }

        /* Nuke everyone */
        RtlZeroMemory(&WsList->Wsle[LastValid + 1], (WsList->LastEntry - LastValid) * sizeof(MMWSLE));
        WsList->LastEntry = LastValid;
    }

    if (LastValid < WsList->FirstDynamic)
    {
        /* Do not mess around with the protected ones */
        return;
    }

    /* See if we should shrink our array */
    if (LastValid == WsList->LastInitializedWsle - (MMWSLE_PER_PAGE - 1))
    {
        PVOID WsleArrayQueue = ALIGN_DOWN_POINTER_BY(&WsList->Wsle[WsList->LastInitializedWsle], PAGE_SIZE);
        PEPROCESS Process = MmGetAddressSpaceOwner(Vm);

        ASSERT(WsList->Wsle[WsList->LastEntry].u1.e1.VirtualPageNumber == ((ULONG_PTR)WsleArrayQueue) >> PAGE_SHIFT);

        /* Kernel address space not supported yet */
        ASSERT(Process != NULL);

        /* Nuke the PTE. This will remove the virtual address from the working set */
        MiDeletePte(MiAddressToPte(WsleArrayQueue), WsleArrayQueue, Process, NULL);
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

    /* Are we de-allocating the WSLE array ? */
    if (WsIndex == (WsList->LastInitializedWsle - (MMWSLE_PER_PAGE - 1)))
		WsList->LastInitializedWsle -= MMWSLE_PER_PAGE;

    /* Insert our entry into the free list */
    if (WsIndex == WsList->LastEntry)
    {
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
            if ((WsIndex - PreviousFreeIndex) < MMWSLE_PER_PAGE)
            	WsleEntry->u1.Free.PreviousFree = WsIndex - PreviousFreeIndex;
            else
            	WsleEntry->u1.Free.PreviousFree = MMWSLE_PREVIOUS_FREE_INVALID;
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
            if ((NextFreeIndex - WsIndex) < MMWSLE_PER_PAGE)
                NextFree->u1.Free.PreviousFree = NextFreeIndex - WsIndex;

            WsleEntry->u1.Free.NextFree = NextFree - WsList->Wsle;
        }
        else
        {
            WsleEntry->u1.Free.NextFree = MMWSLE_NEXT_FREE_INVALID;
        }
    }

    Vm->WorkingSetSize -= PAGE_SIZE;
}

_Requires_exclusive_lock_held_(CurrentProcess->Vm.WorkingSetMutex)
VOID
NTAPI
MiInitializeWorkingSetList(IN PEPROCESS CurrentProcess)
{
    PMMPFN Pfn1;
    PMMPTE sysPte;
    MMPTE tempPte;

    /* We start with the space left behind us */
    MmWorkingSetList->Wsle = (PMMWSLE)(MmWorkingSetList + 1);
    /* Which leaves us this much entries */
    MmWorkingSetList->LastInitializedWsle = ((PAGE_SIZE - sizeof(MMWSL)) / sizeof(MMWSLE)) - 1;

    /* No entry in this list yet ! */
    MmWorkingSetList->LastEntry = 0;
    MmWorkingSetList->HashTable = NULL;
    MmWorkingSetList->HashTableSize = 0;
    MmWorkingSetList->NumberOfImageWaiters = 0;
    MmWorkingSetList->VadBitMapHint = 0;
    MmWorkingSetList->HashTableStart = NULL;
    MmWorkingSetList->HighestPermittedHashAddress = NULL;
    MmWorkingSetList->FirstFree = MMWSLE_NEXT_FREE_INVALID;
    MmWorkingSetList->FirstDynamic = 0;
    MmWorkingSetList->NextSlot = 0;

    /* Map the process working set in kernel space */
    /* FIXME: there should be no need */
    sysPte = MiReserveSystemPtes(1, SystemPteSpace);
    MI_MAKE_HARDWARE_PTE_KERNEL(&tempPte, sysPte, MM_READWRITE, CurrentProcess->WorkingSetPage);
    MI_WRITE_VALID_PTE(sysPte, tempPte);
    CurrentProcess->Vm.VmWorkingSetList = MiPteToAddress(sysPte);

    /* Insert the address we already know: our PDE base and the Working Set List */
#if _MI_PAGING_LEVELS == 4
    MiInsertInWorkingSetList(&CurrentProcess->Vm, (PVOID)PXE_BASE, 0);
#elif _MI_PAGING_LEVELS == 3
    MiInsertInWorkingSetList(&CurrentProcess->Vm, (PVOID)PPE_BASE, 0);
#elif _MI_PAGING_LEVELS == 2
    MiInsertInWorkingSetList(&CurrentProcess->Vm, (PVOID)PDE_BASE, 0);
#endif

#if _MI_PAGING_LEVELS == 4
    MiInsertInWorkingSetList(&CurrentProcess->Vm, MiAddressToPpe(MmWorkingSetList), 0);
#endif
#if _MI_PAGING_LEVELS >= 3
    MiInsertInWorkingSetList(&CurrentProcess->Vm, MiAddressToPde(MmWorkingSetList), 0);
#endif
    MiInsertInWorkingSetList(&CurrentProcess->Vm, MiAddressToPte(MmWorkingSetList), 0);
    MiInsertInWorkingSetList(&CurrentProcess->Vm, MmWorkingSetList, 0);

    /* The rule is that the owner process is always in the FLINK of the PDE's PFN entry */
    Pfn1 = MiGetPfnEntry(CurrentProcess->Pcb.DirectoryTableBase[0] >> PAGE_SHIFT);
    ASSERT(Pfn1->u4.PteFrame == MiGetPfnEntryIndex(Pfn1));
    ASSERT(Pfn1->u1.WsIndex == 0);
    Pfn1->u1.Event = (PKEVENT)CurrentProcess;

    /* Mark this as not initializing anymore */
    MmWorkingSetList->LastEntry = MmWorkingSetList->FirstDynamic - 1;

    /* We can add this to our list */
    ExInterlockedInsertTailList(&MmWorkingSetExpansionHead,
        &CurrentProcess->Vm.WorkingSetExpansionLinks,
        &MmExpansionLock);
}

VOID
NTAPI
MmWorkingSetManager(VOID)
{
       /* For now, do nothing */
}
