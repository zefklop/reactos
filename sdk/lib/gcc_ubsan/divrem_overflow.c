
#include "ubsan.h"

void __ubsan_handle_divrem_overflow(struct SourceLocation* SourceLoc, void* LHS, void* RHS)
{
    ubsan_fail(SourceLoc, "Invalid division", "LHS: %p, RHS %p", LHS, RHS);
}
