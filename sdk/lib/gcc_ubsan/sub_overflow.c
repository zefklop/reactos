
#include "ubsan.h"

void __ubsan_handle_sub_overflow(struct SourceLocation* SourceLoc, void* LHS, void* RHS)
{
    ubsan_fail(SourceLoc, "Integer substraction overflow", "LHS: %p, RHS %p", LHS, RHS);
}
