/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib_private.h"
#define NDEBUG
#include <debug.h>

PVOID CMAPI
HvGetCell(
    PHHIVE RegistryHive,
    HCELL_INDEX CellIndex)
{
    ASSERT(CellIndex != HCELL_NIL);
    return (PVOID)(HvpGetCellHeader(RegistryHive, CellIndex) + 1);
}
