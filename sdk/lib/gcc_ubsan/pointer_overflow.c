
#include "ubsan.h"

void __ubsan_handle_pointer_overflow(struct SourceLocation* SourceLoc, void* Base, void* Result)
{
    ubsan_fail(SourceLoc, "pointer overflow", "Base: %p, Result %p", Base, Result);
}
