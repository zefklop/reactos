
#include "ubsan.h"

void __ubsan_handle_mul_overflow(struct SourceLocation* SourceLoc, void* LHS, void* RHS)
{
    ubsan_fail(SourceLoc, "Integer multiplication overflow", "LHS: %p, RHS %p", LHS, RHS);
}
