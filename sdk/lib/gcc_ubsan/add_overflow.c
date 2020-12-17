
#include "ubsan.h"

void __ubsan_handle_add_overflow(struct SourceLocation* SourceLoc, void* LHS, void* RHS)
{
    ubsan_fail(SourceLoc, "Integer addition overflow", "LHS: %p, RHS %p", LHS, RHS);
}
