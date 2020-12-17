
#include "ubsan.h"

struct TypeMismatchData
{
    struct SourceLocation Loc;
    struct TypeDescriptor* Type;
    unsigned char LogAlignment;
    unsigned char TypeCheckKind;
};

const char *TypeCheckKinds[] = {
    "load of", "store to", "reference binding to", "member access within",
    "member call on", "constructor call on", "downcast of", "downcast of",
    "upcast of", "cast to virtual base of", "_Nonnull binding to"
};

void __ubsan_handle_type_mismatch_v1(struct TypeMismatchData* Data, ULONG_PTR Pointer)
{
    if (!Pointer)
    {
        ubsan_fail(&Data->Loc, "", "%s null pointer", TypeCheckKinds[Data->TypeCheckKind]);
    }
    else if (Pointer & ((1UL << Data->LogAlignment) - 1))
    {
        ubsan_fail(&Data->Loc, "Unaligned access", "%s pointer: %p, LogAlignment %d", TypeCheckKinds[Data->TypeCheckKind], (void*)Pointer, Data->LogAlignment);
    }
    else
    {
        ubsan_fail(&Data->Loc, "Insufficient object size", "%s Pointer: %p, object of type %s", TypeCheckKinds[Data->TypeCheckKind], Pointer, Data->Type->TypeName);
    }
}
