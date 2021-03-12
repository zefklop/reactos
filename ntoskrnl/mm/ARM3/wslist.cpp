/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/wslist.cpp
 * PURPOSE:         Working set list management
 * PROGRAMMERS:     Jérôme Gardou
 */

/* INCLUDES *******************************************************************/
#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "miarm.h"

#include <memory>
#include <scoped_allocator>
#include <unordered_map>
#include <utility>

/* GLOBALS ********************************************************************/
PMMWSL MmWorkingSetList;
KEVENT MmWorkingSetManagerEvent;

/* LOCAL FUNCTIONS ************************************************************/
namespace ntoskrnl
{

template<class T>
struct PMMWSL_alloc
{
private:
    PMMWSL& m_WsList;
public:
    using value_type = T;
    using size_type = size_t;

    template <class U> friend class PMMWSL_alloc;
    template <class U>
    PMMWSL_alloc(const PMMWSL_alloc<U>& u)
        : m_WsList(u.m_WsList) {}

    constexpr PMMWSL_alloc(PMMWSL& WsList)
        : m_WsList(WsList)
    {
    }

    T* allocate(size_type n)
    {
        __debugbreak();
        return NULL;
    }

    void deallocate(T* p, std::size_t n)
    {
        __debugbreak();
    }
};

using PFN_MMWSLE_map = std::unordered_multimap<PFN_NUMBER,
                                               MMWSLE,
                                               std::hash<PFN_NUMBER>,
                                               std::equal_to<PFN_NUMBER>,
                                               PMMWSL_alloc<std::pair<const PFN_NUMBER, MMWSLE>>>;

static
VOID
MiTrimWorkingSet(PMMSUPPORT Vm)
{
}

/* GLOBAL FUNCTIONS ***********************************************************/
extern "C"
{

VOID
NTAPI
MiInsertInWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address,
    _In_ ULONG Protection)
{
    MMWSLE Wsle;
    PFN_NUMBER Page;
    PMMPTE PointerPte = MiAddressToPte(Address);
    PMMPFN Pfn;
    PFN_MMWSLE_map* map = reinterpret_cast<PFN_MMWSLE_map*>(Vm->VmWorkingSetList->HashTable);

    /* Make sure we got a rounded address */
    Address = ALIGN_DOWN_POINTER_BY(Address, PAGE_SIZE);

    /* Make sure we are locking the right things */
    ASSERT(MM_ANY_WS_LOCK_HELD(PsGetCurrentThread()));
    MI_ASSERT_PFN_LOCK_HELD();

    /* Make sure we are adding a paged-in address */
    ASSERT(PointerPte->u.Hard.Valid == 1);

    Page = PFN_FROM_PTE(PointerPte);
    Pfn = MiGetPfnEntry(Page);

    /* The Pfn must be an active one */
    ASSERT(Pfn->u3.e1.PageLocation == ActiveAndValid);

    /* No "ROS PFN" here. */
    ASSERT(MI_IS_ROS_PFN(Pfn) == FALSE);

    /* Setup our Wsle */
    Wsle.u1.Long = 0;
    Wsle.u1.VirtualAddress = Address;
    Wsle.u1.e1.Protection = Protection;

    /* Add it to the map */
    map->insert(std::make_pair(Page, Wsle));

    /* Keep counting */
    Vm->WorkingSetSize += PAGE_SIZE;
    if (Vm->WorkingSetSize > Vm->PeakWorkingSetSize)
        Vm->PeakWorkingSetSize = Vm->WorkingSetSize;
}

VOID
NTAPI
MiRemoveFromWorkingSetList(
    _Inout_ PMMSUPPORT Vm,
    _In_ PVOID Address)
{
}

_Requires_exclusive_lock_held_(WorkingSet->WorkingSetMutex)
VOID
NTAPI
MiInitializeWorkingSetList(IN PMMSUPPORT WorkingSet)
{
    PMMWSL WsList = WorkingSet->VmWorkingSetList;

    /* Initialize our map now, use the space behind us. */
    auto ws_map = new(WsList + 1) PFN_MMWSLE_map{WsList};

    /* Store it there */
    WsList->HashTable = reinterpret_cast<PMMWSLE_HASH>(ws_map);

    /* We start with the space left behind us */
    WorkingSet->VmWorkingSetList->Wsle = (PMMWSLE)(WorkingSet->VmWorkingSetList + 1);
    /* Which leaves us this much entries */
    WorkingSet->VmWorkingSetList->LastInitializedWsle = ((PAGE_SIZE - sizeof(MMWSL)) / sizeof(MMWSLE)) - 1;

    /* No entry in this list yet ! */
    WorkingSet->VmWorkingSetList->LastEntry = MAXULONG;
    WorkingSet->VmWorkingSetList->HashTableSize = 0;
    WorkingSet->VmWorkingSetList->NumberOfImageWaiters = 0;
    WorkingSet->VmWorkingSetList->VadBitMapHint = 0;
    WorkingSet->VmWorkingSetList->HashTableStart = NULL;
    WorkingSet->VmWorkingSetList->HighestPermittedHashAddress = NULL;
    WorkingSet->VmWorkingSetList->FirstFree = MMWSLE_NEXT_FREE_INVALID;
    WorkingSet->VmWorkingSetList->FirstDynamic = 0;
    WorkingSet->VmWorkingSetList->NextSlot = 0;

    /* Initialize our HashTable */


    /* Insert the address we already know: our PDE base and the Working Set List */
    if (MI_IS_PROCESS_WORKING_SET(WorkingSet))
    {
#if _MI_PAGING_LEVELS == 4
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PXE_BASE, 0);
#elif _MI_PAGING_LEVELS == 3
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PPE_BASE, 0);
#elif _MI_PAGING_LEVELS == 2
        MiInsertInWorkingSetList(WorkingSet, (PVOID)PDE_BASE, 0);
#endif

#if _MI_PAGING_LEVELS == 4
        MiInsertInWorkingSetList(WorkingSet, MiAddressToPpe(MmWorkingSetList), 0);
#endif
#if _MI_PAGING_LEVELS >= 3
        MiInsertInWorkingSetList(WorkingSet, MiAddressToPde(MmWorkingSetList), 0);
#endif
        MiInsertInWorkingSetList(WorkingSet, MiAddressToPte(MmWorkingSetList), 0);
        MiInsertInWorkingSetList(WorkingSet, MmWorkingSetList, 0);
    }
    else
    {
        MiInsertInWorkingSetList(WorkingSet, WorkingSet->VmWorkingSetList, 0);
    }

    /* Mark this as not initializing anymore */
    ASSERT(WorkingSet->VmWorkingSetList->FirstDynamic >= 1);
    WorkingSet->VmWorkingSetList->LastEntry = WorkingSet->VmWorkingSetList->FirstDynamic - 1;

    /* We can add this to our list */
    ExInterlockedInsertTailList(&MmWorkingSetExpansionHead, &WorkingSet->WorkingSetExpansionLinks, &MmExpansionLock);
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

} // extern "C"

} // namespace ntoskrnl
