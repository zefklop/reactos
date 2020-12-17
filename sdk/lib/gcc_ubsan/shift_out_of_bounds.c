#include "ubsan.h"

struct ShiftOutOfBoundsData {
  struct SourceLocation Loc;
  const struct TypeDescriptor* LHSType;
  const struct TypeDescriptor* RHSType;
};

void __ubsan_handle_shift_out_of_bounds(struct ShiftOutOfBoundsData* Data, ULONG_PTR LHS, ULONG_PTR RHS)
{
    if (is_negative_value(RHS, Data->RHSType))
    {
        ubsan_fail(&Data->Loc, "Negative bit-shift exponent value", "RHS: %I64d", get_negative_int_value(RHS, Data->RHSType));
        return;
    }

    if (get_positive_int_value(RHS, Data->RHSType) > get_integer_bit_width(Data->LHSType))
    {
        ubsan_fail(&Data->Loc, "Bit-shift exponent value too large", "RHS: %U64d, LHS of type %s, bit width: %d",
                   get_positive_int_value(RHS, Data->RHSType), Data->LHSType->TypeName, get_integer_bit_width(Data->LHSType));
        return;
    }

    if (is_negative_value(LHS, Data->LHSType))
    {
        ubsan_fail(&Data->Loc, "Bit shifting negative value", "LHS: %I64d", get_negative_int_value(LHS, Data->LHSType));
        return;
    }
    else
    {
        ULONGLONG RHSValue = get_positive_int_value(RHS, Data->RHSType);
        ULONGLONG LHSValue = get_positive_int_value(LHS, Data->LHSType);

        /* Make a readable output */
        if ((RHSValue < 0xFFFFFFFFULL) && (LHSValue < 0xFFFFFFFFULL))
        {
            ubsan_fail(&Data->Loc, "Bit shift overflow", "Left shifting %x by %d cannot be represented by type %s",
                (ULONG)LHSValue, (ULONG)RHSValue, Data->LHSType->TypeName);
        }
        else if (RHSValue < 0xFFFFFFFFULL)
        {
            ubsan_fail(&Data->Loc, "Bit shift overflow", "Left shifting %x%08x by %d cannot be represented by type %s",
                (ULONG)(LHSValue >> 32), (ULONG)LHSValue, (ULONG)RHSValue, Data->LHSType->TypeName);
        }
        else
        {
            ubsan_fail(&Data->Loc, "Bit shift overflow", "Left shifting %x%08x by %x%08x cannot be represented by type %s",
                (ULONG)(LHSValue >> 32), (ULONG)LHSValue, (ULONG)(RHSValue >> 32), (ULONG)RHSValue, Data->LHSType->TypeName);
        }
    }
}
